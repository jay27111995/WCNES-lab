#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/adxl345.h" 
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"


#define LED_INT_ONTIME        CLOCK_SECOND
#define ACCM_READ_INTERVAL    CLOCK_SECOND/100

#define SIGNAL_INTRUSION_DETECTED 1
#define SIGNAL_BUTTON_PRESSED 1

/* Declare our "main" process, the client process*/
PROCESS(client_process, "Clicker client");
PROCESS(accel_process, "Accel process");
PROCESS(led_process, "LED handling process");

/* The client process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&client_process, &accel_process, &led_process);


static process_event_t ledOff_event;
static process_event_t client_send_msg_event;

/* LED handling process */
static struct etimer ledETimer;
PROCESS_THREAD(led_process, ev, data) {
  PROCESS_BEGIN();
  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == ledOff_event);
    etimer_set(&ledETimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ledETimer));
    leds_off(LEDS_RED + LEDS_GREEN + LEDS_BLUE);
  }
  PROCESS_END();
}

#if 0 
void
accm_movement_detected(uint8_t reg){
  
  leds_on(LEDS_BLUE);
  process_post(&led_process, ledOff_event, NULL);
  process_post(&client_process, client_send_msg_event, NULL);  

  //printf("~~[%u] Freefall detected! (0x%02X) -- ", ((uint16_t) clock_time())/128, reg);
  //print_int(reg);
}
#endif 

/* Accelerometer process */
static struct etimer et;


PROCESS_THREAD(accel_process, ev, data) {
  PROCESS_BEGIN();
  {
    int16_t x;
    static int16_t x_old; 
    const int16_t error_val = 50; 

    /* Register the event used for lighting up an LED when interrupt strikes. */
    ledOff_event = process_alloc_event();
    client_send_msg_event = process_alloc_event();

    /* Start and setup the accelerometer with default values, eg no interrupts enabled. */
    accm_init();
    x_old = accm_read_axis(X_AXIS);

#if 0 
    /* Register the callback functions for each interrupt */
    ACCM_REGISTER_INT1_CB(accm_movement_detected);

    /* Set what strikes the corresponding interrupts. Several interrupts per pin is 
      possible. For the eight possible interrupts, see adxl345.h and adxl345 datasheet. */
    accm_set_irq(ADXL345_INT_ACTIVITY, ADXL345_INT_DISABLE);
#endif 

    while (1) {
	    x = accm_read_axis(X_AXIS);
	    printf("x: %d \n", x);
        
        if ((x_old + error_val) > x) {
            process_post(&client_process, client_send_msg_event, NULL);  

            leds_on(LEDS_BLUE);
            process_post(&led_process, ledOff_event, NULL);
        }

        x_old = x;

      etimer_set(&et, ACCM_READ_INTERVAL);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
#if 0 
static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
}
#endif

/* Our main process. */
PROCESS_THREAD(client_process, ev, data) {
	PROCESS_BEGIN();

    static uint8_t msg1 = SIGNAL_INTRUSION_DETECTED;
#if 0 
    static char msg1[] = "hej";
    static char msg2[] = "hej";

	/* Activate the button sensor. */
	SENSORS_ACTIVATE(button_sensor);
#endif 

	/* Initialize NullNet */
	nullnet_buf = (uint8_t *)&payload;
	nullnet_len = sizeof(payload);
	nullnet_set_input_callback(recv);

	/* Loop forever. */
	while (1) {
		/* Wait until an event occurs. If the event has
		 * occured, ev will hold the type of event, and
		 * data will have additional information for the
		 * event. In the case of a sensors_event, data will
		 * point to the sensor that caused the event.
		 * Here we wait until the button was pressed. */
#if 0 
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
			data == &button_sensor);
#endif 
		PROCESS_WAIT_EVENT_UNTIL(ev == client_send_msg_event); 

		leds_toggle(LEDS_RED);
		/* Copy the string "hej" into the packet buffer. */
		memcpy(nullnet_buf, &msg1, sizeof(msg1));
    nullnet_len = sizeof(msg1);

		/* Send the content of the packet buffer using the
		 * broadcast handle. */
		NETSTACK_NETWORK.output(NULL);
	}

	PROCESS_END();
}
