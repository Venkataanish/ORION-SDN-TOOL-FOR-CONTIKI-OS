/*---------------------------------------------------------------------------*/
/*                                MY-MQTT                                    */
/*---------------------------------------------------------------------------*/

#include "utile.h"

/*NOW THE CONTIKI PROCESS STARTS*/
PROCESS(orion_mqtt_process, "Orion mqtt process");
/*---------------------------------------------------------------------------*/

/* A function which returns length of the IPv6 address
 * param -
 * buffer - The string buffer into which ip address is stored
 * buffer_length - A variable into which the length will be stored
 * addr - The address whose you want to get length of
 */
int get_length(char *buffer, uint8_t buffer_length, const uip_ipaddr_t *addr)
{
  uint16_t a;
  uint8_t length = 0;
  int i, f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i = i+ 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        length += snprintf(&buffer[length], buffer_length - length, "::");
      }
    } else {
       if(i > 0) {
        length += snprintf(&buffer[length], buffer_length - length, ":");
      }
       else if(f > 0) {
               f = -1;
             }
      length += snprintf(&buffer[length], buffer_length - length, "%x", a);
    }
  }

  return length;
}


/*---------------------------------------------------------------------------*/
/* A function which is used to get the RSSI of the previous packet in case the
 *  source route doesn't match with the default route
 *  return nothing*/
static void echo_reply_handler(uip_ipaddr_t *source, uint8_t ttl, uint8_t *data, uint16_t datalen)
{
  if(uip_ip6addr_cmp(source, uip_ds6_defrt_choose())) {
    def_rt_rssi = sicslowpan_get_last_rssi();
  }
}
/*---------------------------------------------------------------------------*/
/* A function to switch the CC2538 LED's off
 * returns nothing */
static void turn_off_led_cc2538(void *d)
{
  leds_off(STATUS_LED);
}

/*---------------------------------------------------------------------------*/
/* A function which is used to handle various error cases in the publishing
 * returns nothing */
static void handle_publish(const char *topic, uint16_t topic_len, const uint8_t *chunk, uint16_t chunk_len)
{
  /* If the length of the topic is less than 19 then print an error message*/
  if(topic_len < 19 || topic_len>BUFFER_SIZE) {
    printf("Usage: mosquitto_pub -h fd00::1 -m <message> -t iot-2/evt/%s/fmt/json ",topic);
    return;
  }

  /* Two checks to see if the publish format is of the form iot-2/evt/<topic>/fmt/json . If not the usage is printed on the screen */
  if(strncmp(&topic[0],"iot-2",5)|| strncmp(&topic[6],"evt",3)){
	  printf("Usage: mosquitto_pub -h fd00::1 -m <message> -t iot-2/evt/%s/fmt/json ",topic);
	  return;
  }
  if(strncmp(&topic[topic_len-4],"json",4) || strncmp(&topic[topic_len-8],"fmt",3)){
	  printf("Usage: mosquitto_pub -h fd00::1 -m <message> -t iot-2/evt/%s/fmt/json ",topic);
	  	  return;
  }

  /* Turning the CC2538 LED's on and off by checking the chunk
   * if message = 1 TURN the LED on
   * if message = 0 TURN the LED off
   */
  if(strcmp(&topic[10], "leds") == 0) {
    if(chunk[0] == '1') {
      leds_on(LEDS_RED);
    } else if(chunk[0] == '0') {
      leds_off(LEDS_RED);
    }
    return;
  }
}

/*---------------------------------------------------------------------------*/
/*A function which is used to handle the events
 * MQTT_EVENT_CONNECTED    - CONNECTION STATE
 * MQTT_EVENT_PUBLISH      - PUBLISHING STATE
 * MQTT_EVENT_DISCONNECTED - DISCONNECTED STATE
 *
 * returns nothing
 */
static void mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
  switch(event) {
  case MQTT_EVENT_CONNECTED: {
    printf("Connection is established. Ready to Publish/Subscribe");
    timer_set(&connection_life, CONNECTION_STABLE_TIME);
    state = CONNECTED_STATE;
    break;
  }

  case MQTT_EVENT_PUBLISH: {
    data_ptr = data;
    uint16_t data_len = strlen(data_ptr->topic);
    printf("Event published with topic %s\n",data_ptr->topic);
    handle_publish(data_ptr->topic, data_len, data_ptr->payload_chunk,
                data_ptr->payload_length);
    break;
  }
  case MQTT_EVENT_DISCONNECTED: {
      printf("Connection is broken because of %u\n", *((mqtt_event_t *)data));
      state = DISCONNECTED_STATE;
      process_poll(&orion_mqtt_process);
      break;
    }

  default:
    break;
  }
}

/*A function to check if the length of the topic or client id is less than 0
 * or greater than the buffer size permitted.
 * It returns
 * 0 - if error
 * 1 - if no error
 */
static int len_check(int len){
	if(len<0 || len >BUFFER_SIZE){
		return 0;
	}
	return 1;
}
/*---------------------------------------------------------------------------*/
/*A function to construct the "published" topic in a string format"
 * It returns
 * 0 - if error
 * 1 - if no error
 * */
static int pub_topic_to_string(void)
{
  int length = snprintf(pub_topic, BUFFER_SIZE, "iot-2/evt/%s/fmt/json", config.event_type_id);

  if(len_check(length)){
  return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/*A function to construct the "subscribed" topic in a string format
 * It returns
 * 0 - if error
 * 1 - if no error
 * */
static int sub_topic_to_string(void)
{
  int length = snprintf(sub_topic, BUFFER_SIZE, "iot-2/cmd/%s/fmt/json", config.cmd_type);

  if(len_check(length)){
  return 1;
  }
  return 0;
}

/*---------------------------------------------------------------------------*/
/*A function to construct the client's IPv6 address in a string format
 * It returns
 * 0 - if error
 * 1 - if no error
 * */
static int client_id_to_string(void)
{
  int length = snprintf(client_id, BUFFER_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x",
                     config.org_id, config.type_id,
                     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                     linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
                     linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

  if(len_check(length)){
  return 1;
  }
  return 0;
}

/*---------------------------------------------------------------------------*/
/* A function to update the configuration state. It schedules a timer event and
 * sets the state of the machine to init state if there is no error.
 *
 * If inside the error state, the state is exited only if we get a new config.
 * returns nothing
 */
static void update_config(void)
{
  if(!sub_topic_to_string() ||!pub_topic_to_string() || !client_id_to_string()) {
    state = CONFIG_ERROR_STATE;
    return;
  }
  /* Reset the counter */
  seq_num = 0;

  state = INITIALIZATION_STATE;
  etimer_set(&publish_periodic_timer, 0);

  return;
}
/*---------------------------------------------------------------------------*/
/* A function to populate configuration with default values
 * returns
 * 1 - if configuration initialization is successful
 *  */

static int init_config()
{
  memset(&config, 0, sizeof(mqtt_client_config_t));

  memcpy(config.org_id, DEFAULT_ORG_ID, strlen(DEFAULT_ORG_ID));
  memcpy(config.type_id, DEFAULT_TYPE_ID, strlen(DEFAULT_TYPE_ID));
  memcpy(config.auth_token, DEFAULT_AUTH_TOKEN, strlen(DEFAULT_AUTH_TOKEN));
  memcpy(config.event_type_id, DEFAULT_EVENT_TYPE_ID, strlen(DEFAULT_EVENT_TYPE_ID));
  memcpy(config.broker_ip, broker_ip, strlen(broker_ip));
  memcpy(config.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);

  config.broker_port = DEFAULT_BROKER_PORT;
  config.pub_interval = DEFAULT_PUBLISH_INTERVAL;
  config.def_rt_ping_interval = DEFAULT_RSSI_MEAS_INTERVAL;

  return 1;
}

/*---------------------------------------------------------------------------*/
/* A function which is used to manipulate remaining string length and string
 * and buffer pointer as well
 * returns nothing
 */
static void manipulate_length(char * buff_ptr, int remaining, int length){
	remaining -= length;
	buff_ptr += length;
}

/*---------------------------------------------------------------------------*/
/* A function which is used to subscribe to topic in IBM quickstart format
 * returns nothing */
static void subscribe(void)
{
  mqtt_subscribe(&connect, NULL, sub_topic, MQTT_QOS_LEVEL_0);

}

/*---------------------------------------------------------------------------*/
/* A function which is used to publish to topic in IBM quickstart format
 * I have made it to publish the following cc2538 values
 * Default RSSI value
 * On-Chip temperature
 * VDD voltage level
 * returns nothing */
static void publish(void)
{
  int length;
  int remaining = MQTT_BUFFER_SIZE;

  uint32_t mymqtt_length;
  seq_num++;

  buf_ptr = mymqtt_buffer;
  char def_rt_str[64];

  length = snprintf(buf_ptr, remaining,"{"
                 "\"d\":{"
                 "\"myName\":\"%s\","
                 "\"Seq #\":%d,"
                 "\"Uptime (sec)\":%lu",
                 BOARD_STRING, seq_num, clock_seconds());

  if(!len_check(length)) {
    return;
  }

  manipulate_length(buf_ptr,remaining,length);

  memset(def_rt_str, 0, sizeof(def_rt_str));
  get_length(def_rt_str, sizeof(def_rt_str), uip_ds6_defrt_choose());

  length = snprintf(buf_ptr, remaining, ",\"Def Route\":\"%s\",\"RSSI (dBm)\":%d", def_rt_str, def_rt_rssi);

  if(!len_check(length)) {
      return;
    }

  manipulate_length(buf_ptr,remaining,length);

  length = snprintf(buf_ptr, remaining, ",\"On-Chip Temp (mC)\":%d", cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED));

  if(!len_check(length)) {
      return;
    }
  manipulate_length(buf_ptr,remaining,length);

  length = snprintf(buf_ptr, remaining, ",\"VDD3 (mV)\":%d", vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED));

  if(!len_check(length)) {
        return;
      }
  manipulate_length(buf_ptr,remaining,length);


  length = snprintf(buf_ptr, remaining, "}}");

  if(!len_check(length)) {
          return;
        }
  mymqtt_length = strlen(mymqtt_buffer);
  mqtt_publish(&connect, NULL, pub_topic, (uint8_t *)mymqtt_buffer, mymqtt_length , MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
}

/*---------------------------------------------------------------------------*/
/* A helper function to connect to broker using mqtt_connect function call
 * returns nothing */
static void mqtt_broker_connect(void)
{
  char *host = config.broker_ip;

  uint16_t port = config.broker_port;
  uint16_t keepalive = config.pub_interval *3;

  mqtt_connect(&connect, host, port, keepalive);

  state = CONNECTING_STATE;
}

/*---------------------------------------------------------------------------*/
/* A function which us used to get the global IPv6 address for the open mote
 * and send a ping message to its immediate neighbor to check proper working
 * of the open mote cc2538's connection to the  mosquitto broker
 * returns nothing */
static void get_global_address(void)
{
  if(uip_ds6_get_global(ADDR_PREFERRED) == NULL) {
    return;
  }

  uip_icmp6_send(uip_ds6_defrt_choose(), ICMP6_ECHO_REQUEST, 0, ECHO_REQ_PAYLOAD_LEN);
}

/*---------------------------------------------------------------------------*/
/* A function which computes which state the connection is in. The different
 * types of states are
 * INITIALIZATION STATE - if in this state, mqtt connection is registered
 * REGISTRATION STATE - if the cc2538 has been registered with a public IP we
 *                      connect to the broker or we wait for a timeout period
 *                      and then connect again
 * CONNECTING STATE   - it is in the connecting state, will get connected
 * CONNECTED STATE    - if in this state, change the state to registered state
 * PUBLISHING STATE   - if in connected state, first subscribe and change the
 * 						state to publishing. Otherwise, simply publish.
 * 						Handles cases where there is loss of packets due to
 * 						disconnection to the broker or some network delay
 * DISCONNECTED STATE - if in this state, it tries to connect RECONNECT_ATTEMPT
 * 						number of times or it WILL RETRY FOREVER
 * 						Changes the state to registered after the connection is
 * 						established
 * CONFIG ERROR STATE - Config error has occurred. State set to NEWCONFIG.
 * 						It occurs when there is an error in the Client ID,
 * 						subscribe topic or publish topic.
 * ERROR STATE        - CC2538 has reached the Error State. State set to
 * 						NEWCONFIG
 *
 * If none of the states is reached, a CC2538TIMEOUT period is waited for and
 * the entire process is repeated again
 * */
static void state_machine(void)
{
  clock_time_t interval;

  switch(state) {
  case INITIALIZATION_STATE:
    /* If we have just been configured register MQTT connection */
    mqtt_register(&connect, &orion_mqtt_process, client_id, mqtt_event,
                  MAX_TCP_SEGMENT_SIZE);

    state = REGISTRATION_STATE;

    connect.auto_reconnect = 0;
    connect_attempt = 1;


    printf("Initializing state\n");

  case REGISTRATION_STATE:
    if(uip_ds6_get_global(ADDR_PREFERRED) != NULL) {
      printf("Registered Successfully\n");
      get_global_address();
      mqtt_broker_connect();
    } else {
      leds_on(STATUS_LED);
      ctimer_set(&ct, NO_NET_LED_DURATION, turn_off_led_cc2538, NULL);
    }
    etimer_set(&publish_periodic_timer, NET_CONNECT_PERIODIC);
    return;
    break;

  case CONNECTING_STATE:
    leds_on(STATUS_LED);
    ctimer_set(&ct, CC2538_LED_TIME, turn_off_led_cc2538, NULL);
    printf("Connecting State\n");
    break;

  case CONNECTED_STATE:
    /* Don't subscribe unless we are a registered device */
    if(strncasecmp(config.org_id, QUICKSTART, strlen(config.org_id)) == 0) {
      printf("Connected state\n");
      state = PUBLISHING_STATE;
    }

  case PUBLISHING_STATE:
    /* If the timer expired, the connection is stable. */
    if(timer_expired(&connection_life)) {
      connect_attempt = 0;
    }

    if(mqtt_ready(&connect) && connect.out_buffer_sent) {
      /* Connected. Publish */
      if(state == CONNECTED_STATE) {
        subscribe();
        state = PUBLISHING_STATE;
      } else {
        leds_on(STATUS_LED);
        ctimer_set(&ct, CC2538_PUB_TIME, turn_off_led_cc2538, NULL);
        publish();
      }
      etimer_set(&publish_periodic_timer, config.pub_interval);

      printf("Publishing State\n");
      /* Return here so we don't end up rescheduling the timer */
      return;
    } else {

      printf("Connection to broker down or there is some network delay. Please wait\n");
    }
    break;
  case DISCONNECTED_STATE:
    printf("Disconnected State\n");
    if(connect_attempt < RECONNECT_ATTEMPTS ||
       RECONNECT_ATTEMPTS == RETRY_FOREVER) {
      mqtt_disconnect(&connect);
      connect_attempt++;

      interval = connect_attempt < MAX_CONNECT_ATTEMPT ? RECONNECT_INTERVAL << connect_attempt :
        RECONNECT_INTERVAL << MAX_CONNECT_ATTEMPT;

      printf("Disconnected. This is the Attempt number %u trying to connect\n", connect_attempt);

      etimer_set(&publish_periodic_timer, interval);

      state = REGISTRATION_STATE;
      return;
    } else {
      state = ERROR_STATE;
      printf("MAX_CONNECT_ATTEMPT exceeded. Now abortion of the connection happening\n");
    }
    break;

  case CONFIG_ERROR_STATE:
    printf("Config error state\n");
    state = NEWCONFIG_STATE;
    return;

  case ERROR_STATE:
	  leds_on(STATUS_LED);
	  printf("Error state");
	  state = NEWCONFIG_STATE;
	  return;

  default:
      break;

  }
  etimer_set(&publish_periodic_timer, CC2538_TIMEOUT);
}
/*---------------------------------------------------------------------------*/
/* NOW THE CONTIKI PROCESS THREAD BEGINS. This is same for all the boards.
 * It checks if the initial configuration is proper.
 * One of the folllowing happens:
 *
 * If the reset button is pressed as a publish trigger:
 * First check if the state of the CC2538 is proper( ie., one of the six
 * states listed in the state_machine function. If it is proper it moves to
 * registered state.
 *
 * In the interval of publish periodic timer, check if the state of CC2538 is
 * in one of the 7 states. Then do things accordingly. Preferable publish.
 *
 * In the interval of the echo request, the global address of the CC2538 mote
 * is obtained before proceeding further.
 *  */
PROCESS_THREAD(orion_mqtt_process, ev, data)
{

  PROCESS_BEGIN();

  printf("ORION MQTT Process\n");






  if(init_config() != 1) {
    PROCESS_EXIT();
  }

  update_config();

  def_rt_rssi = 0x8000000;

  uip_icmp6_echo_reply_callback_add(&echo_reply_notification,echo_reply_handler);
  etimer_set(&echo_request_timer, config.def_rt_ping_interval);

  while(1) {

    PROCESS_YIELD();

    if(ev == sensors_event && data == PUBLISH_TRIGGER) {
      if(state == ERROR_STATE) {
        connect_attempt = 1;
        state = REGISTRATION_STATE;
      }
    }

    if((ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer) ||
       ev == PROCESS_EVENT_POLL ||
       (ev == sensors_event && data == PUBLISH_TRIGGER)) {
      state_machine();
    }

    if(ev == PROCESS_EVENT_TIMER && data == &echo_request_timer) {
      get_global_address();
      etimer_set(&echo_request_timer, config.def_rt_ping_interval);
    }
  }

  PROCESS_END();
}

