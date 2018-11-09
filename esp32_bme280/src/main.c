#include "mgos.h"
#include "mgos_mqtt.h"
#include "mgos_bme280.h"

#define MINUTE 60000
#define PUBLISH_INTERVAL 5000

static void publish(const char *fmt, ...) {
    char message[200];
    struct json_out json_message = JSON_OUT_BUF(message, sizeof(message));

    va_list ap;
    va_start(ap, fmt);
    int n = json_vprintf(&json_message, fmt, ap);
    va_end(ap);

    mgos_mqtt_pub(mgos_sys_config_get_mqtt_publish(), message, n, 1, 0);
}

static void publish_timer_callback(void *sensor_arg) {
    struct mgos_bme280* sensor = (struct mgos_bme280*) sensor_arg;
    struct mgos_bme280_data data = {
        .temp=0.0,
        .press=0.0,
        .humid=0.0
    };
    mgos_bme280_read(sensor, &data);
    LOG(LL_INFO, ("Location: %s, Temperature: %f, Pressure: %f, Humidity: %f", mgos_sys_config_get_location_name(), data.temp, data.press, data.humid));
    time_t t = time(0);
    struct tm* timeinfo = localtime(&t);
    char timestamp[24];
    strftime(timestamp, sizeof (timestamp), "%FT%TZ", timeinfo);
    
    publish("{measurement: %Q, tags: {rom: %Q}, time: %Q, fields: {Temperature: %f, Pressure: %f, Humidity: %f}}", "bme280", mgos_sys_config_get_location_name(), timestamp, data.temp, data.press, data.humid);
}

enum mgos_app_init_result mgos_app_init(void) {
    struct mgos_bme280* sensor = mgos_bme280_i2c_create(0x76);
    mgos_set_timer(PUBLISH_INTERVAL, true, publish_timer_callback, sensor);

    return MGOS_APP_INIT_SUCCESS;
}

// {
//     "measurement": "dummy",
//     "tags": {
//         "tag1": "some tag",
//         "tag2": "some other tag"
//     },
//     "time": "2009-11-10T23:00:00Z",
//     "fields": {
//         "Float_value": 0.64,
//         "Int_value": 3,
//         "String_value": "Text",
//         "Bool_value": True
//     }
// }
