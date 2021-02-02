#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"


#define LED_INT_ONTIME (CLOCK_SECOND * 10) // Keep the LEDs On for 10s 


#define SIGNAL_INTRUSION_DETECTED 1
#define SIGNAL_BUTTON_PRESSED 2


static process_event_t led0Off_event;
static process_event_t led1Off_event;
static process_event_t led2Off_event;

/* Declare our "main" process, the basestation_process */
PROCESS(basestation_process, "Clicker basestation");
PROCESS(led0_process, "LED 0 handling process");
PROCESS(led1_process, "LED 1 handling process");
PROCESS(led2_process, "LED 2 handling process");

/* The basestation process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&basestation_process, &led0_process, &led1_process, &led2_process);

/*---------------------------------------------------------------------------*/
/* When posted an ledOff event, the LEDs will switch off after LED_INT_ONTIME.
      static process_event_t ledOff_event;
      ledOff_event = process_alloc_event();
      process_post(&led_process, ledOff_event, NULL);
*/

static struct etimer led0ETimer;
PROCESS_THREAD(led0_process, ev, data) {
  PROCESS_BEGIN();
  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == led0Off_event);
    etimer_set(&led0ETimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led0ETimer));
    leds_off(0);
  }
  PROCESS_END();
}

static struct etimer led1ETimer;
PROCESS_THREAD(led1_process, ev, data) {
  PROCESS_BEGIN();
  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == led1Off_event);
    etimer_set(&led1ETimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led1ETimer));
    leds_off(1);
  }
  PROCESS_END();
}

static struct etimer led2ETimer;
PROCESS_THREAD(led2_process, ev, data) {
  PROCESS_BEGIN();
  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == led2Off_event);
    etimer_set(&led2ETimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led2ETimer));
    leds_off(2);
  }
  PROCESS_END();
}


/* Holds the number of packets received. */
static int count = 0;

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
static uint8_t msg_intrusion_flag = 0;
static uint8_t msg_button_flag = 0;

static struct etimer led_3_Timer;
static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
    count++;
    
    uint8_t *mesg = NULL;
    mesg = (uint8_t *) data;
    
    if (etimer_expired(&led_3_Timer)) {
        msg_button_flag = 0;
        msg_intrusion_flag = 0;
    }

    /* 0bxxxxx allows us to write binary values */
    /* for example, 0b10 is 2 */
    if (*mesg == 1) 
    {
        leds_on(0);
        msg_intrusion_flag = 1;
        process_post(&led0_process, led0Off_event, NULL);
    }
    else 
    {
        leds_on(1);
        msg_button_flag = 1;
        process_post(&led1_process, led1Off_event, NULL);
    }

    if (msg_intrusion_flag && msg_button_flag && !etimer_expired(&led_3_Timer)) {
        leds_on(2);
        process_post(&led2_process, led2Off_event, NULL);
        msg_button_flag = 1;
    }

    etimer_set(&led_3_Timer, LED_INT_ONTIME);
    printf("Msg recieved from client\n");
}

/* Our main process. */
PROCESS_THREAD(basestation_process, ev, data) {
	PROCESS_BEGIN();

    /* Register the event used for lighting up an LED when interrupt strikes. */
    led0Off_event = process_alloc_event();
    led1Off_event = process_alloc_event();
    led2Off_event = process_alloc_event();

	/* Initialize NullNet */
	nullnet_set_input_callback(recv);

	PROCESS_END();
}
