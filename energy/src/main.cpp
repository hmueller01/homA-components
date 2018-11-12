/*
 * Holger Mueller
 * 2018/11/09
 * Modified from Tobias Lorenz to send HomA framework MQTT messages
 * based on specific OBIS data.
 *
 * Copyright (C) 2018 Tobias Lorenz.
 * Contact: tobias.lorenz@gmx.net
 *
 * This file is part of Tobias Lorenz's Toolkit.
 *
 * Commercial License Usage
 * Licensees holding valid commercial licenses may use this file in
 * accordance with the commercial license agreement provided with the
 * Software or, alternatively, in accordance with the terms contained in
 * a written agreement between you and Tobias Lorenz.
 *
 * GNU General Public License 3.0 Usage
 * Alternatively, this file may be used under the terms of the GNU
 * General Public License version 3.0 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.  Please review the following information to
 * ensure the GNU General Public License version 3.0 requirements will be
 * met: http://www.gnu.org/copyleft/gpl.html.
 */

/* C includes */
#include <unistd.h>
#ifdef WITH_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

/* C++ includes */
#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mosquittopp.h>
#include <sstream>
#include <stdexcept>
#include <thread>

/* project internal includes */
#include "SML.h"
#include "MqttClient.h"

/** abort the main loop */
std::atomic<bool> abortLoop;

/** handle SIGTERM */
void signalHandler(int /*signum*/)
{
    abortLoop = true;
}

/** main function */
int main(int argc, char ** argv)
{
    /* default parameters */
    std::string host = "localhost";
    int port = 1883;
    int qos = 1;
    std::string topic = "/devices/123456-energy/controls";
    std::string id = "sml2mqtt";
    std::string username = "";
    std::string password = "";
    std::string device = "/dev/vzir0";

    /* evaluate command line parameters */
    int c;
    while ((c = getopt (argc, argv, "h:p:q:t:i:u:P:d:")) != -1) {
        switch (c) {
        case 'h':
            host = optarg;
            break;
        case 'p':
            port = std::stoul(optarg);
            break;
        case 'q':
            qos = std::stoul(optarg);
            break;
        case 't':
            topic = optarg;
            break;
        case 'i':
            id = optarg;
            break;
        case 'u':
            username = optarg;
            break;
        case 'P':
            password = optarg;
            break;
        case 'd':
            device = optarg;
            break;
        default:
            std::cerr << "Usage: sml2mqtt [-h host] [-p port] [-q qos] [-t topic] [-i id] [-u username] [-P password] [-d device]" << std::endl;
            return EXIT_FAILURE;
        }
    }

    /* register signal handler */
    abortLoop = false;
    signal(SIGTERM, signalHandler);

    /* mosquitto constructor */
    if (mosqpp::lib_init() != MOSQ_ERR_SUCCESS) {
        std::cerr << "main: lib_init failed" << std::endl;
        return EXIT_FAILURE;
    }

    /* start MqttClient */
    mqttClient() = new MqttClient(host.c_str(), port, qos, topic.c_str(), id.c_str(), username.c_str(), password.c_str(), "#/set");


    // check if MQTT client is available
    if (!mqttClient()) {
        return EXIT_FAILURE;
    }

    // setup HomA meta data
    /*
        {'obis': '1-0:16.7.0*255', 'scale': 1, 'unit': ' W', 'topic': 'Current Power'},
    	{'obis': '1-0:1.8.0*255', 'scale': 1000, 'unit': ' kWh', 'topic': 'Total Energy'}
    */
	mqttClient()->setTopic("Current Power/meta/type", "text");
	mqttClient()->setTopic("Current Power/meta/unit", " W");
	mqttClient()->setTopic("Current Power/meta/order", "1");
	mqttClient()->setTopic("Total Energy/meta/type", "text");
	mqttClient()->setTopic("Total Energy/meta/unit", " kWh");
	mqttClient()->setTopic("Total Energy/meta/order", "2");

    /* init all channels */
    SML sml(device);
    if (!sml.is_open()) {
        return -1;
    }

#ifdef WITH_SYSTEMD
    /* systemd notify */
    sd_notify(0, "READY=1");
#endif

    /* start publish loop */
    while(!abortLoop) {
#ifdef WITH_SYSTEMD
        /* systemd notify */
        sd_notify(0, "WATCHDOG=1");
#endif

        /* read channels and publish via MQTT */
        // listen on the serial device, this call is blocking.
        sml.transport_listen();
    }

    /* delete resources */
    delete mqttClient();

    /* mosquitto destructor */
    if (mosqpp::lib_cleanup() != MOSQ_ERR_SUCCESS) {
        std::cerr << "main: lib_cleanup failed" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
