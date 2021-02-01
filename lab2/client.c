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

/* Declare our "main" process, the client process*/
//PROCESS(client_process, "Clicker client");
PROCESS(accel_process, "Accel process");
PROCESS(led_process, "LED handling process");

/* The client process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&client_process, &accel_process, &led_process);

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


void
accm_movement_detected(uint8_t reg){
  
  leds_on(LEDS_BLUE);
  process_post(&led_process, ledOff_event, NULL);
  //process_post(&client_process, clientSendMsg_event, NULL);  

  //printf("~~[%u] Freefall detected! (0x%02X) -- ", ((uint16_t) clock_time())/128, reg);
  //print_int(reg);
}

/* Accelerometer process */
static struct etimer et;
PROCESS_THREAD(accel_process, ev, data) {
  PROCESS_BEGIN();
  {
    int16_t x, y, z;

    /* Register the event used for lighting up an LED when interrupt strikes. */
    ledOff_event = process_alloc_event();

    /* Start and setup the accelerometer with default values, eg no interrupts enabled. */
    accm_init();

    /* Register the callback functions for each interrupt */
    ACCM_REGISTER_INT1_CB(accm_movement_detected);

    /* Set what strikes the corresponding interrupts. Several interrupts per pin is 
      possible. For the eight possible interrupts, see adxl345.h and adxl345 datasheet. */
    accm_set_irq(ADXL345_INT_ACTIVITY);

    while (1) {
	    x = accm_read_axis(X_AXIS);
	    y = accm_read_axis(Y_AXIS);
	    z = accm_read_axis(Z_AXIS);
	    printf("x: %d y: %d z: %d\n", x, y, z);

      etimer_set(&et, ACCM_READ_INTERVAL);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    }
  }
  PROCESS_END();
}

#if 0 
/*---------------------------------------------------------------------------*/

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
}

/* Our main process. */
PROCESS_THREAD(client_process, ev, data) {
    static char msg1[] = "hej";
    static char msg2[] = "hej";

	PROCESS_BEGIN();

	/* Activate the button sensor. */
	SENSORS_ACTIVATE(button_sensor);

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
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
			data == &button_sensor);

		leds_toggle(LEDS_RED);
		/* Copy the string "hej" into the packet buffer. */
		memcpy(nullnet_buf, &payload, sizeof(payload));
    nullnet_len = sizeof(payload);

		/* Send the content of the packet buffer using the
		 * broadcast handle. */
		NETSTACK_NETWORK.output(NULL);
	}

	PROCESS_END();
}
#endif 
