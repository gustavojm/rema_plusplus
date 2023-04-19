#ifndef REMA_H_
#define REMA_H_

class rema {
public:
    static void control_enabled_set(bool status);

    static bool control_enabled_get();

    static void probe_enabled_set(bool status);

    static bool probe_enabled_get();

    static void stall_control_set(bool status);

    static bool stall_control_get();

    void lamp_pwr_set(bool status);

    static bool control_enabled;
    static bool probe_enabled;
    static bool stall_detection;

};

#endif /* REMA_H_ */
