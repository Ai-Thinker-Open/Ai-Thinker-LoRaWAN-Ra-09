/**
  * @file    lora_transparent_at.c
  * @brief   ra08 lora_transparent_lpuart_ADDR compatible AT + transparent LoRa
  *
  * Command table and at_process()/cmd_process() logic ported from ra08-demo
  * lora_transparent_lpuart_ADDR/src/lora_test.c
  */
  #include "lora_transparent_at.h"
  #include "platform.h"
  #include "main.h"
  #include "radio.h"
  #include "timer_if.h"
  #include "stm32_lpm.h"
  #include "utilities_def.h"
  #include "stm32_seq.h"
  #include "stm32_timer.h"
  #include "stm32_adv_trace.h"
  
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <stdbool.h>
  
  /* --- ra08-compatible constants --- */
  #define LORA_SETTING_ADDR       0x0803F000UL
  #define ARGC_LIMIT              16
  #define LORA_AT_MODE            0
  #define LORA_TRANSPARENT_MODE   1
  #define AT_ERROR                "+CMD ERROR:"
  #define ATCMD_SIZE              (255 * 2 + 18)
  #define QUERY_CMD               0x01
  #define EXECUTE_CMD             0x02
  #define DESC_CMD                0x03
  #define SET_CMD                 0x04
  #define MAX_BUFFER_LENGTH       253
  #define LORA_SYMBOL_TIMEOUT     5
  #define LORA_PREAMBLE_LENGTH    8
  #define TRANSPARENT_IDLE_MS     15U
  
  #define RF_FREQUENCY_DEFAULT    470625000UL
  #define TX_OUTPUT_POWER_DEFAULT 22
  #define LORA_BANDWIDTH_DEFAULT  0
  #define LORA_DR_DEFAULT         3
  #define LORA_CODERATE_DEFAULT   1
  
  #define AT_REPLY(...) \
    do { \
      while (UTIL_ADV_TRACE_OK != \
             UTIL_ADV_TRACE_COND_FSend(VLEVEL_ALWAYS, T_REG_OFF, TS_OFF, __VA_ARGS__)) \
      { \
      } \
    } while (0)
  
  typedef struct
  {
    uint16_t length;
    uint8_t buffer[MAX_BUFFER_LENGTH];
  } lora_buffer_t;
  
  typedef enum
  {
    LORA_IDLE = 0,
    LORA_TXDONE,
    LORA_TXTIMEOUT,
    LORA_RXDONE,
    LORA_RXTIMEOUT,
    LORA_RXERROR,
  } lora_status_t;
  
  typedef struct
  {
    char *cmd;
    int (*fn)(int opt, int argc, char *argv[]);
  } at_cmd_t;
  
  typedef struct
  {
    int8_t save_flag;
    uint32_t freq;
    int8_t power;
    uint32_t bandwidth;
    uint32_t datarate;
    uint8_t coderate;
    uint16_t preambleLen;
    uint8_t iqInverted;
  } lora_cfg_Params_t;
  
  static uint16_t local_addr = 0;
  static uint16_t target_addr = 1;
  static int lora_mode = LORA_AT_MODE;
  
  static uint8_t atcmd[ATCMD_SIZE];
  static uint16_t atcmd_index;
  static uint8_t at_cmd_done;
  static uint8_t at_saw_cr;
  
  static lora_buffer_t LoRa_RX_Buffer;
  static lora_buffer_t LoRa_TX_Buffer;
  static lora_status_t LoRaStatus = LORA_IDLE;
  static int16_t last_rx_rssi;
  static int8_t last_rx_snr;
  
  static uint8_t radio_send_done = 0;
  static uint32_t last_uart_tick;
  static uint8_t uart_activity;
  static uint8_t radio_ready = 0;
  static uint8_t uart_busy_notified = 0;
  
  typedef enum
  {
    RADIO_JOB_NONE = 0,
    RADIO_JOB_INIT,
    RADIO_JOB_RECONFIG,
  } radio_job_t;
  
  static volatile radio_job_t radio_job = RADIO_JOB_NONE;
  
  static uint8_t pending_tx_buf[MAX_BUFFER_LENGTH + 8U];
  static uint16_t pending_tx_len;
  
  static UTIL_TIMER_Object_t TransparentTxTimer;
  
  static RadioEvents_t TestRadioEvents;
  static lora_cfg_Params_t lora_Params = {
    0, RF_FREQUENCY_DEFAULT, TX_OUTPUT_POWER_DEFAULT, LORA_BANDWIDTH_DEFAULT,
    LORA_DR_DEFAULT, LORA_CODERATE_DEFAULT, LORA_PREAMBLE_LENGTH, 0
  };
  
  static int test_case_ctxcw(int opt, int argc, char *argv[]);
  static int test_case_ctx(int opt, int argc, char *argv[]);
  static int test_case_csleep(int opt, int argc, char *argv[]);
  static int test_case_cstdby(int opt, int argc, char *argv[]);
  static int test_case_addr_set(int opt, int argc, char *argv[]);
  static int test_case_txaddr_set(int opt, int argc, char *argv[]);
  
  static const at_cmd_t g_at_table[] = {
    {"+CTXADDR", &test_case_txaddr_set},
    {"+CSLEEP", &test_case_csleep},
    {"+CSTDBY", &test_case_cstdby},
    {"+CADDR", &test_case_addr_set},
    {"+CTXCW", &test_case_ctxcw},
    {"+CTX", &test_case_ctx},
  };
  
  #define AT_TABLE_SIZE (sizeof(g_at_table) / sizeof(at_cmd_t))
  
  static void event_process(void);
  
  static uint8_t CheckSum8(uint8_t *pData, uint8_t len)
  {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++)
    {
      sum += pData[i];
    }
    sum = (uint8_t)(~sum + 1U);
    return sum;
  }
  
  static uint32_t map_spreading_factor(uint8_t dr)
  {
    uint8_t sf = (uint8_t)(12U - dr);
    if (sf < 5U) sf = 5U;
    if (sf > 12U) sf = 12U;
    return sf;
  }
  
  static int flash_save_lora_cfg(void)
  {
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0;
    uint64_t *src = (uint64_t *)&lora_Params;
    uint32_t addr = LORA_SETTING_ADDR;
    uint32_t words = (sizeof(lora_cfg_Params_t) + 7U) / 8U;
  
    HAL_FLASH_Unlock();
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Page = (LORA_SETTING_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE;
    erase.NbPages = 1;
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK)
    {
      HAL_FLASH_Lock();
      return -1;
    }
    for (uint32_t i = 0; i < words; i++)
    {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, src[i]) != HAL_OK)
      {
        HAL_FLASH_Lock();
        return -1;
      }
      addr += 8U;
    }
    HAL_FLASH_Lock();
    return 0;
  }
  
  static void test_case_at(void)
  {
    AT_REPLY("OK\r\n");
  }
  
  static void resume_radio_rx(void)
  {
    if (radio_ready != 0U && radio_job == RADIO_JOB_NONE)
    {
      Radio.Rx(0);
    }
  }
  
  static void radio_notify_process(void)
  {
    UTIL_SEQ_SetTask(1 << CFG_SEQ_Task_LoraProcess, CFG_SEQ_Prio_0);
  }
  
  static void cmd_notify_process(void)
  {
    UTIL_SEQ_SetTask(1 << CFG_SEQ_Task_Vcom, CFG_SEQ_Prio_1);
  }
  
  static bool is_lora_busy(void)
  {
    if (radio_job != RADIO_JOB_NONE)
    {
      return true;
    }
    if (radio_send_done != 0U)
    {
      return true;
    }
    if (pending_tx_len != 0U)
    {
      return true;
    }
    return false;
  }
  
  static void schedule_radio_setup(void)
  {
    if (radio_ready != 0U)
    {
      radio_job = RADIO_JOB_RECONFIG;
    }
    else
    {
      radio_job = RADIO_JOB_INIT;
    }
    radio_notify_process();
  }
  
  static void transparent_tx_timer_cb(void *context)
  {
    (void)context;
    UTIL_SEQ_SetTask(1 << CFG_SEQ_Task_Vcom, CFG_SEQ_Prio_1);
  }
  
  /* Radio IRQ callbacks: only set status + wake WaitEvt (no Radio.* / no AT_REPLY here). */
  static void OnTxDone(void)
  {
    LoRaStatus = LORA_TXDONE;
    UTIL_SEQ_SetEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
  }
  
  static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
  {
    LoRaStatus = LORA_RXDONE;
    last_rx_rssi = rssi;
    last_rx_snr = snr;
    if (size > MAX_BUFFER_LENGTH)
    {
      size = MAX_BUFFER_LENGTH;
    }
    memcpy(LoRa_RX_Buffer.buffer, payload, size);
    LoRa_RX_Buffer.length = size;
    radio_notify_process();
  }
  
  static void OnTxTimeout(void)
  {
    LoRaStatus = LORA_TXTIMEOUT;
    UTIL_SEQ_SetEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
  }
  
  static void OnRxTimeout(void)
  {
    LoRaStatus = LORA_RXTIMEOUT;
    radio_notify_process();
  }
  
  static void OnRxError(void)
  {
    LoRaStatus = LORA_RXERROR;
    radio_notify_process();
  }
  
  static void radio_events_init(void)
  {
    TestRadioEvents.TxDone = OnTxDone;
    TestRadioEvents.RxDone = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError = OnRxError;
  }
  
  static void apply_lora_radio_config(bool iq_inverted)
  {
    /* SetTxConfig/SetRxConfig LoRa bandwidth arg is index 0=125k, 1=250k, 2=500k (see PingPong). */
    uint32_t bw = lora_Params.bandwidth;
    if (bw > 2U)
    {
      bw = LORA_BANDWIDTH_DEFAULT;
      AT_REPLY("bw error, set to 125kHz\r\n");
    }
    uint32_t sf = map_spreading_factor((uint8_t)lora_Params.datarate);
  
    Radio.SetChannel(lora_Params.freq);
    Radio.SetTxConfig(MODEM_LORA, lora_Params.power, 0, bw, sf, lora_Params.coderate,
                      lora_Params.preambleLen, false, true, 0, 0, iq_inverted, 60000);
    Radio.SetRxConfig(MODEM_LORA, bw, sf, lora_Params.coderate, 0, lora_Params.preambleLen,
                      LORA_SYMBOL_TIMEOUT, false, 0, true, 0, 0, iq_inverted, true);
    Radio.SetMaxPayloadLength(MODEM_LORA, MAX_BUFFER_LENGTH);
    Radio.SetPublicNetwork(false);
  }
  
  static void lora_setting(void)
  {
    radio_events_init();
    Radio.Init(&TestRadioEvents);
    AT_REPLY("LoRa Config(freq: %u, dr: %u, bw:%u, cr: %u, power: %d)\r\n",
             (unsigned)lora_Params.freq, (unsigned)lora_Params.datarate,
             (unsigned)lora_Params.bandwidth, (unsigned)lora_Params.coderate, (int)lora_Params.power);
    apply_lora_radio_config(lora_Params.iqInverted != 0);
    LoRa_RX_Buffer.length = 0;
    LoRa_TX_Buffer.length = 0;
  }
  
  static void request_radio_ready(void)
  {
    if (radio_ready != 0U && radio_job == RADIO_JOB_NONE)
    {
      return;
    }
    if (radio_job == RADIO_JOB_NONE)
    {
      schedule_radio_setup();
    }
  }
  
  static void radio_process_jobs(void)
  {
    if (radio_job == RADIO_JOB_INIT && radio_ready == 0U)
    {
      lora_setting();
      radio_ready = 1U;
      radio_job = RADIO_JOB_NONE;
      if (lora_mode == LORA_TRANSPARENT_MODE)
      {
        resume_radio_rx();
      }
      cmd_notify_process();
    }
    else if (radio_job == RADIO_JOB_RECONFIG && radio_ready != 0U)
    {
      apply_lora_radio_config(lora_Params.iqInverted != 0);
      radio_job = RADIO_JOB_NONE;
      if (lora_mode == LORA_TRANSPARENT_MODE)
      {
        resume_radio_rx();
      }
      cmd_notify_process();
    }
  }
  
  static void enter_deepsleep(void)
  {
    AT_REPLY("enter deepsleep...\r\n");
    UTIL_LPM_EnterLowPower();
    AT_REPLY("leave deepsleep...\r\n");
  }
  
  static void at_process(void)
  {
    char *ptr = NULL;
    char *rxcmd = NULL;
    int argc = 0;
    int index = 0;
    char *argv[ARGC_LIMIT];
    int ret = -1;
  
    if (atcmd_index < 2)
    {
      atcmd_index = 0;
      memset(atcmd, 0xFF, ATCMD_SIZE);
      return;
    }
  
    if (atcmd[0] != 'A' || atcmd[1] != 'T')
    {
      ret = -1;
      goto at_end;
    }
  
    /* Bare AT (CR/LF stripped in atcmd_finish_line): reply OK */
    if (atcmd_index == 2)
    {
      test_case_at();
      atcmd_index = 0;
      memset(atcmd, 0xFF, ATCMD_SIZE);
      return;
    }
  
    rxcmd = (char *)(atcmd + 2);
  
    if (rxcmd[0] == '\0')
    {
      ret = -1;
      goto at_end;
    }
  
    for (index = 0; index < (int)AT_TABLE_SIZE; index++)
    {
      int cmd_len = (int)strlen(g_at_table[index].cmd);
      if (!strncmp((const char *)rxcmd, g_at_table[index].cmd, (size_t)cmd_len))
      {
        ptr = (char *)rxcmd + cmd_len;
        break;
      }
    }
  
    if (index >= (int)AT_TABLE_SIZE || !g_at_table[index].fn)
    {
      ret = -1;
      goto at_end;
    }
  
    if ((ptr[0] == '?') && (ptr[1] == '\0'))
    {
      ret = g_at_table[index].fn(QUERY_CMD, argc, argv);
    }
    else if (ptr[0] == '\0')
    {
      ret = g_at_table[index].fn(EXECUTE_CMD, argc, argv);
    }
    else if (ptr[0] == ' ')
    {
      argv[argc++] = ptr;
      ret = g_at_table[index].fn(EXECUTE_CMD, argc, argv);
    }
    else if ((ptr[0] == '=') && (ptr[1] == '?') && (ptr[2] == '\0'))
    {
      ret = g_at_table[index].fn(DESC_CMD, argc, argv);
    }
    else if (ptr[0] == '=')
    {
      ptr += 1;
      for (char *p = ptr; *p != '\0'; p++)
      {
        if (*p == '\r' || *p == '\n')
        {
          *p = '\0';
          break;
        }
      }
      char *str = strtok((char *)ptr, ",");
      while (str && argc < ARGC_LIMIT)
      {
        argv[argc++] = str;
        str = strtok(NULL, ",");
      }
      ret = g_at_table[index].fn(SET_CMD, argc, argv);
    }
    else
    {
      ret = -1;
    }
  
  at_end:
    if (ret == -1)
    {
      AT_REPLY("\r\n%s%x\r\n", AT_ERROR, 1);
    }
    atcmd_index = 0;
    at_saw_cr = 0U;
    memset(atcmd, 0xFF, ATCMD_SIZE);
  }
  
  static int test_case_cstdby(int opt, int argc, char *argv[])
  {
    (void)opt;
    if (argc != 1)
    {
      return -1;
    }
    uint8_t stdby_mode = (uint8_t)strtol(argv[0], NULL, 0);
    if (radio_ready == 0U || radio_job != RADIO_JOB_NONE)
    {
      return -1;
    }
    if (stdby_mode == 0)
    {
      Radio.Sleep();
    }
    else
    {
      Radio.Sleep();
    }
    enter_deepsleep();
    return 0;
  }
  
  static int test_case_csleep(int opt, int argc, char *argv[])
  {
    (void)opt;
    if (argc != 1)
    {
      return -1;
    }
    uint8_t sleep_mode = (uint8_t)strtol(argv[0], NULL, 0);
    if (radio_ready == 0U || radio_job != RADIO_JOB_NONE)
    {
      return -1;
    }
    if (sleep_mode == 0)
    {
      Radio.Sleep();
    }
    else
    {
      Radio.Sleep();
    }
    enter_deepsleep();
    return 0;
  }
  
  static int test_case_ctx(int opt, int argc, char *argv[])
  {
    if (SET_CMD != opt)
    {
      return -1;
    }
    if (argc != 6)
    {
      return -1;
    }
  
    uint32_t freq = (uint32_t)strtoul(argv[0], NULL, 0);
    uint8_t dr = (uint8_t)strtol(argv[1], NULL, 0);
    uint8_t bw = (uint8_t)strtol(argv[2], NULL, 0);
    uint8_t cr = (uint8_t)strtol(argv[3], NULL, 0);
    uint8_t pwr = (uint8_t)strtol(argv[4], NULL, 0);
    uint8_t iqInverted = (uint8_t)strtol(argv[5], NULL, 0);
    if(bw > 9U){
      bw = 0U;
      AT_REPLY("bw error, set to 125kHz\r\n");
    }
    if (freq > 1000000000UL || freq < 100000000UL) {
      freq = 470625000UL;
      AT_REPLY("freq error, set to 470625000Hz\r\n");
    }
    if (dr > 7U) {
      dr = 3U;
      AT_REPLY("dr error, set to 3\r\n");
    } 
    if (cr > 4U || cr < 1U) {
      cr = 1U;
      AT_REPLY("cr error, set to 1\r\n");
    }
    if (pwr > 22U) {
      pwr = 22U;
      AT_REPLY("pwr error, set to 22dBm\r\n");
    }
    if (iqInverted > 1U) {
      iqInverted = 0U;
      AT_REPLY("iqInverted error, set to 0\r\n");
    }
  
    lora_Params.freq = freq;
    lora_Params.datarate = dr;
    lora_Params.bandwidth = bw;
    lora_Params.coderate = cr;
    lora_Params.power = (int8_t)pwr;
    lora_Params.iqInverted = iqInverted;
  
    AT_REPLY("config radio params data(freq: %u, dr: %u, bw:%u, cr: %u, power: %u iqInverted: %u)\r\n",
             (unsigned long)freq, dr, bw, cr, pwr, iqInverted);
  
    lora_mode = LORA_TRANSPARENT_MODE;
    LoRa_RX_Buffer.length = 0;
    LoRa_TX_Buffer.length = 0;
    lora_Params.save_flag = 1;
    if (flash_save_lora_cfg() != 0)
    {
      return -1;
    }
    schedule_radio_setup();
    AT_REPLY("LORA_TRANSPARENT_MODE\r\n");
    return 0;
  }
  
  static int test_case_ctxcw(int opt, int argc, char *argv[])
  {
    (void)opt;
    if (argc < 2)
    {
      return -1;
    }
    uint32_t freq = (uint32_t)strtoul(argv[0], NULL, 0);
    uint8_t pwr = (uint8_t)strtol(argv[1], NULL, 0);
    if (pwr > 22U) return -1;
  
    if (radio_ready == 0U || radio_job != RADIO_JOB_NONE)
    {
      return -1;
    }
    AT_REPLY("Start to txcw (freq: %lu, power: %udb)\r\n", (unsigned long)freq, pwr);
    Radio.SetTxContinuousWave(freq, pwr, 0xFFFFU);
    return 0;
  }
  
  static int test_case_addr_set(int opt, int argc, char *argv[])
  {
    (void)opt;
    if (argc != 1)
    {
      return -1;
    }
    local_addr = (uint16_t)strtol(argv[0], NULL, 0);
    AT_REPLY("set local address: %u \r\nOK\r\n", (unsigned)local_addr);
    return 0;
  }
  
  static int test_case_txaddr_set(int opt, int argc, char *argv[])
  {
    (void)opt;
    if (argc != 1)
    {
      return -1;
    }
    target_addr = (uint16_t)strtol(argv[0], NULL, 0);
    AT_REPLY("set target address: %u \r\nOK\r\n", (unsigned)target_addr);
    return 0;
  }
  
  static void event_process(void)
  {
    uint16_t rx_to_addr;
    uint16_t rx_from_addr;
    uint8_t check_sum;
  
    switch (LoRaStatus)
    {
      case LORA_TXDONE:
        AT_REPLY("TXDONE\r\n");
        LoRaStatus = LORA_IDLE;
        resume_radio_rx();
        break;
  
      case LORA_TXTIMEOUT:
        AT_REPLY("TXTIMEOUT\r\n");
        LoRaStatus = LORA_IDLE;
        resume_radio_rx();
        break;
  
      case LORA_RXDONE:
        AT_REPLY("rssi:%d snr:%d\r\n", (int)last_rx_rssi, (int)last_rx_snr);
        if (LoRa_RX_Buffer.length < 6U)
        {
          AT_REPLY("RXDONE ERROR DATA\r\n");
          LoRaStatus = LORA_IDLE;
          resume_radio_rx();
          break;
        }
        rx_from_addr = ((uint16_t)LoRa_RX_Buffer.buffer[1] << 8) + LoRa_RX_Buffer.buffer[2];
        rx_to_addr = ((uint16_t)LoRa_RX_Buffer.buffer[3] << 8) + LoRa_RX_Buffer.buffer[4];
        check_sum = CheckSum8(LoRa_RX_Buffer.buffer, LoRa_RX_Buffer.length - 1U);
        if (LoRa_RX_Buffer.buffer[0] == 0xAA && local_addr == rx_to_addr &&
            check_sum == LoRa_RX_Buffer.buffer[LoRa_RX_Buffer.length - 1U])
        {
          AT_REPLY("RXDONE from %u size:%u\r\n", (unsigned)rx_from_addr, (unsigned)LoRa_RX_Buffer.length);
          for (uint16_t i = 5; i < LoRa_RX_Buffer.length - 1U; i++)
          {
            AT_REPLY("%c", LoRa_RX_Buffer.buffer[i]);
          }
          AT_REPLY("\r\n");
        }
        else
        {
          AT_REPLY("RXDONE ERROR DATA\r\n");
        }
        memset(LoRa_RX_Buffer.buffer, 0, MAX_BUFFER_LENGTH);
        LoRa_RX_Buffer.length = 0;
        LoRaStatus = LORA_IDLE;
        resume_radio_rx();
        break;
  
      case LORA_RXTIMEOUT:
        AT_REPLY("RXTIMEOUT\r\n");
        LoRaStatus = LORA_IDLE;
        resume_radio_rx();
        break;
  
      case LORA_RXERROR:
        AT_REPLY("RXERROR\r\n");
        LoRaStatus = LORA_IDLE;
        resume_radio_rx();
        break;
  
      default:
        break;
    }
  }
  
  static void transparent_do_send(void)
  {
    uint8_t tx_len;
  
    if (pending_tx_len == 0U || radio_send_done != 0U)
    {
      return;
    }
    if (radio_ready == 0U || radio_job != RADIO_JOB_NONE)
    {
      return;
    }
  
    tx_len = (uint8_t)pending_tx_len;
    pending_tx_len = 0U;
    radio_send_done = 1U;
    uart_busy_notified = 0U;
  
    apply_lora_radio_config(lora_Params.iqInverted != 0);
  
    LoRaStatus = LORA_IDLE;
    UTIL_SEQ_ClrEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
    
    Radio.Send(pending_tx_buf, tx_len);
  
    /* Block LoRa task until TxDone/TxTimeout (driver TxTimeoutTimer, default 60000 ms). */
    UTIL_SEQ_WaitEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
  
    radio_send_done = 0U;
    event_process();
  }
  
  static bool transparent_buffer_is_plus_escape(void)
  {
    if (LoRa_TX_Buffer.length < 3U)
    {
      return false;
    }
    if (LoRa_TX_Buffer.buffer[0] != '+' ||
        LoRa_TX_Buffer.buffer[1] != '+' ||
        LoRa_TX_Buffer.buffer[2] != '+')
    {
      return false;
    }
    for (uint16_t i = 3U; i < LoRa_TX_Buffer.length; i++)
    {
      uint8_t c = LoRa_TX_Buffer.buffer[i];
      if (c != '\r' && c != '\n')
      {
        return false;
      }
    }
    return true;
  }
  
  static void transparent_try_send(void)
  {
    uint8_t addr[5];
    uint16_t tran_index = 0;
  
    if (lora_mode != LORA_TRANSPARENT_MODE)
    {
      return;
    }
    if (is_lora_busy())
    {
      memset(LoRa_TX_Buffer.buffer, 0, MAX_BUFFER_LENGTH);
      LoRa_TX_Buffer.length = 0;
      return;
    }
    if (radio_ready == 0U || radio_job != RADIO_JOB_NONE)
    {
      request_radio_ready();
      return;
    }
    if (LoRa_TX_Buffer.length == 0U)
    {
      return;
    }
    if (uart_activity != 0U)
    {
      uint32_t now = TIMER_IF_GetTimerValue();
      if ((now - last_uart_tick) < TRANSPARENT_IDLE_MS)
      {
        return;
      }
      uart_activity = 0U;
    }
  
    if (transparent_buffer_is_plus_escape())
    {
      lora_mode = LORA_AT_MODE;
      AT_REPLY("AT_MODE\r\n");
      memset(LoRa_TX_Buffer.buffer, 0, MAX_BUFFER_LENGTH);
      LoRa_TX_Buffer.length = 0;
      return;
    }
  
    if (LoRa_TX_Buffer.length > 247U)
    {
      AT_REPLY("data over flow\r\n");
      LoRa_TX_Buffer.length = 247U;
    }
  
    addr[0] = 0xAA;
    addr[1] = (uint8_t)((local_addr >> 8) & 0xFFU);
    addr[2] = (uint8_t)(local_addr & 0xFFU);
    addr[3] = (uint8_t)((target_addr >> 8) & 0xFFU);
    addr[4] = (uint8_t)(target_addr & 0xFFU);
  
    for (uint8_t i = 0; i < 5U; i++)
    {
      pending_tx_buf[tran_index++] = addr[i];
    }
    for (uint16_t i = 0; i < LoRa_TX_Buffer.length; i++)
    {
      pending_tx_buf[tran_index++] = LoRa_TX_Buffer.buffer[i];
    }
    pending_tx_buf[tran_index] = CheckSum8(pending_tx_buf, tran_index);
    tran_index++;
    pending_tx_len = tran_index;
  
    memset(LoRa_TX_Buffer.buffer, 0, MAX_BUFFER_LENGTH);
    LoRa_TX_Buffer.length = 0;
    radio_notify_process();
  }
  
  static bool atcmd_is_allowed(char c)
  {
    return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || c == '?' || c == '+' ||
            c == ':' || c == '=' || c == ' ' || c == ',');
  }
  
  static void atcmd_finish_line(void)
  {
    while (atcmd_index > 0U &&
           (atcmd[atcmd_index - 1U] == '\r' || atcmd[atcmd_index - 1U] == '\n'))
    {
      atcmd_index--;
    }
  
    if (atcmd_index == 0U)
    {
      return;
    }
  
    if (atcmd_index >= ATCMD_SIZE)
    {
      memset(atcmd, 0xFF, ATCMD_SIZE);
      atcmd_index = 0;
      return;
    }
  
    atcmd[atcmd_index] = '\0';
    at_cmd_done = 1U;
  }
  
  static void process_uart_byte(uint8_t cmd)
  {
    if (lora_mode == LORA_AT_MODE)
    {
      if (cmd == '\r')
      {
        at_saw_cr = 1U;
      }
      else if (cmd == '\n')
      {
        if (at_saw_cr != 0U)
        {
          atcmd_finish_line();
        }
        at_saw_cr = 0U;
      }
      else if (atcmd_is_allowed((char)cmd))
      {
        at_saw_cr = 0U;
        if (atcmd_index >= ATCMD_SIZE)
        {
          memset(atcmd, 0xFF, ATCMD_SIZE);
          atcmd_index = 0;
          return;
        }
        atcmd[atcmd_index++] = cmd;
      }
      else
      {
        at_saw_cr = 0U;
      }
    }
    else if (lora_mode == LORA_TRANSPARENT_MODE)
    {
      if (is_lora_busy())
      {
        if (uart_busy_notified == 0U)
        {
          AT_REPLY("BUSY\r\n");
          uart_busy_notified = 1U;
        }
        return;
      }
  
      if (LoRa_TX_Buffer.length < MAX_BUFFER_LENGTH)
      {
        LoRa_TX_Buffer.buffer[LoRa_TX_Buffer.length++] = cmd;
      }
      uart_activity = 1U;
      last_uart_tick = TIMER_IF_GetTimerValue();
      UTIL_TIMER_Stop(&TransparentTxTimer);
      UTIL_TIMER_SetPeriod(&TransparentTxTimer, TRANSPARENT_IDLE_MS);
      UTIL_TIMER_Start(&TransparentTxTimer);
    }
  }
  
  void LoraTransparent_PreInit(void)
  {
    lora_cfg_Params_t saved;
    memcpy(&saved, (const void *)LORA_SETTING_ADDR, sizeof(lora_cfg_Params_t));
  
    if (saved.save_flag == 1)
    {
      memcpy(&lora_Params, &saved, sizeof(lora_cfg_Params_t));
    }
  
    atcmd_index = 0;
    at_cmd_done = 0;
    at_saw_cr = 0U;
    memset(atcmd, 0xFF, ATCMD_SIZE);
    UTIL_TIMER_Create(&TransparentTxTimer, TRANSPARENT_IDLE_MS, UTIL_TIMER_ONESHOT,
                      transparent_tx_timer_cb, NULL);
  }
  
  void LoraTransparent_PostInit(void)
  {
    if (((const lora_cfg_Params_t *)LORA_SETTING_ADDR)->save_flag == 1)
    {
      AT_REPLY("use last config\r\n");
    }
    else
    {
      AT_REPLY("use default config\r\n");
    }
    AT_REPLY("AT_MODE\r\n");
  }
  
  void LoraTransparent_OnUartByte(uint8_t byte)
  {
    process_uart_byte(byte);
  }
  
  void LoraTransparent_CmdProcess(void)
  {
    if (lora_mode == LORA_AT_MODE && at_cmd_done != 0U)
    {
      at_process();
      at_cmd_done = 0U;
    }
    if (!is_lora_busy())
    {
      transparent_try_send();
    }
  }
  
  void LoraTransparent_RadioProcess(void)
  {
    radio_process_jobs();
    transparent_do_send();
    event_process();
  }
  