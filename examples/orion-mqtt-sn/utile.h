#ifndef _UTILE_H_
#define _UTILE_H_

#include "contiki-conf.h"

#include "mqtt.h"

#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

#include "serial-shell.h"



#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"
#include "lib/random.h"

#include "sys/pt.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/leds.h"

#include "tcp-socket.h"

#include "lib/assert.h"
#include "lib/list.h"
#include "sys/cc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "rpl/rpl-private.h"

#include "net/rpl/rpl.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"

#include "sys/etimer.h"
#include "sys/ctimer.h"

#include "lib/sensors.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/cc2538-sensors.h"

#include <string.h>


/* I have kept the case for quickstart. It can be used for mqtt-sn communication only over the localhost. For communication over the internet
 * a NAT64 bridge is to be used
 */

static const char *broker_ip =      "fd00::1";
#define DEFAULT_ORG_ID              "quickstart"

/*---------------------------------------------------------------------------*
 * TIMEOUT DURATION FOR C2538 HARDWARE
 */
#define CC2538_TIMEOUT     (CLOCK_SECOND >> 1)
/*---------------------------------------------------------------------------*/
/* LED ON DURATION FOR CC2538 HARDWARE */
#define CC2538_LED_TIME    (CLOCK_SECOND >> 2)

/* PUBLISH TIME FOR CC2538 HARDWARE */
#define CC2538_PUB_TIME    (CLOCK_SECOND)

#define CONNECTION_STABLE_TIME     (CLOCK_SECOND * 5)

#define RETRY_FOREVER              0xFF
#define RECONNECT_INTERVAL         (CLOCK_SECOND * 2)
#define RECONNECT_ATTEMPTS         RETRY_FOREVER

static struct timer connection_life;
static uint8_t connect_attempt;


static uint8_t state;
#define INITIALIZATION_STATE  0
#define REGISTRATION_STATE    1
#define CONNECTING_STATE      2
#define CONNECTED_STATE       3
#define PUBLISHING_STATE      4
#define DISCONNECTED_STATE    5
#define NEWCONFIG_STATE       6
#define CONFIG_ERROR_STATE 0xFE
#define ERROR_STATE        0xFF
/*---------------------------------------------------------------------------*/
#define CONFIG_ORG_ID_LEN        32
#define CONFIG_TYPE_ID_LEN       32
#define CONFIG_AUTH_TOKEN_LEN    32
#define CONFIG_EVENT_TYPE_ID_LEN 32
#define CONFIG_CMD_TYPE_LEN       8
#define CONFIG_IP_ADDR_STR_LEN   64
/*---------------------------------------------------------------------------*/
#define RSSI_MEASURE_INTERVAL_MAX 10000
#define RSSI_MEASURE_INTERVAL_MIN     5

#define PUBLISH_INTERVAL_MAX      10000
#define PUBLISH_INTERVAL_MIN          5

#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)
#define NO_NET_LED_DURATION         (NET_CONNECT_PERIODIC >> 1)

#define DEFAULT_TYPE_ID             "cc2538"
#define DEFAULT_AUTH_TOKEN          "AUTHZ"
#define DEFAULT_EVENT_TYPE_ID       "status"
#define DEFAULT_SUBSCRIBE_CMD_TYPE  "+"
#define DEFAULT_BROKER_PORT         1883
#define DEFAULT_PUBLISH_INTERVAL    (30 * CLOCK_SECOND)
#define DEFAULT_KEEP_ALIVE_TIMER    60
#define DEFAULT_RSSI_MEAS_INTERVAL  (CLOCK_SECOND * 30)

#define PUBLISH_TRIGGER &button_sensor

#define MQTT_DEMO_STATUS_LED      LEDS_GREEN
#define MQTT_DEMO_PUBLISH_TRIGGER &button_right_sensor



#define ECHO_REQ_PAYLOAD_LEN   20

/*---------------------------------------------------------------------------*/

PROCESS_NAME(orion_mqtt_process);
AUTOSTART_PROCESSES(&orion_mqtt_process);

typedef struct mqtt_data_packet_handle{
   char* dev_id;
   char* chip_temperature;
   char* VDD_level;
}mqtt_data_packet_handle_t;

typedef struct mqtt_client_config {
  char org_id[CONFIG_ORG_ID_LEN];
  char type_id[CONFIG_TYPE_ID_LEN];
  char auth_token[CONFIG_AUTH_TOKEN_LEN];
  char event_type_id[CONFIG_EVENT_TYPE_ID_LEN];
  char broker_ip[CONFIG_IP_ADDR_STR_LEN];
  char cmd_type[CONFIG_CMD_TYPE_LEN];
  clock_time_t pub_interval;
  int def_rt_ping_interval;
  uint16_t broker_port;
} mqtt_client_config_t;

mqtt_client_config_t config;
/*---------------------------------------------------------------------------*/
#define MAX_TCP_SEGMENT_SIZE    32
/*---------------------------------------------------------------------------*/
#define STATUS_LED LEDS_GREEN
/*---------------------------------------------------------------------------*/
#define MAX_CONNECT_ATTEMPT 3

#define BUFFER_SIZE 64
static char client_id[BUFFER_SIZE];
static char pub_topic[BUFFER_SIZE];
static char sub_topic[BUFFER_SIZE];

#define MQTT_BUFFER_SIZE 512
struct mqtt_connection connect;
static char mymqtt_buffer[MQTT_BUFFER_SIZE];

#define QUICKSTART "quickstart"

static struct mqtt_message *data_ptr = 0;
static struct etimer publish_periodic_timer;
static struct ctimer ct;
static char *buf_ptr;
static uint16_t seq_num = 0;

static struct uip_icmp6_echo_reply_notification echo_reply_notification;
static struct etimer echo_request_timer;
static int def_rt_rssi = 0;

/*                       Function Declaration                                */

int get_length(char *, uint8_t , const uip_ipaddr_t *);
static void echo_reply_handler(uip_ipaddr_t *, uint8_t , uint8_t *, uint16_t );
static void turn_off_led_cc2538(void *);
static void handle_publish(const char *, uint16_t , const uint8_t *, uint16_t );
static void mqtt_event(struct mqtt_connection *, mqtt_event_t , void *);
static int len_check(int len);
static int pub_topic_to_string(void);
static int sub_topic_to_string(void);
static int client_id_to_string(void);
static void update_config(void);
static int init_config();
static void manipulate_length(char * , int, int );
static void subscribe(void);
static void publish(void);
static void mqtt_broker_connect(void);
static void get_global_address(void);
static void state_machine(void);

static void
call_event(struct mqtt_connection *conn,
           mqtt_event_t event,
           void *data);

static void
handle_unsuback(struct mqtt_connection *conn);

static void
parse_publish_vhdr(struct mqtt_connection *conn,
                   uint32_t *pos,
                   const uint8_t *input_data_ptr,
                   int input_data_len);

#endif


