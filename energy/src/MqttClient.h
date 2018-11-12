/*
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

#pragma once

/* C++ includes */
#include <map>
#include <mutex>
#include <string>
#include <mosquittopp.h>

class MqttClient : private mosqpp::mosquittopp
{
public:
    MqttClient(const char * host, int port, int qos, const char * baseTopic, const char * id, const char * username, const char * password, const char * subscribeTopic);
    virtual ~MqttClient();

    /**
     * set topic, and publish on change
     *
     * @param topic[in] topic
     * @param payload payload
     */
    void setTopic(std::string topic, std::string payload);

    /**
     * get topic, that was set before
     *
     * @param[in] topic topic
     * @param[in] default default value, if topic is not available
     * @return payload
     */
    std::string getTopic(std::string topic, std::string defaultValue = "") const;

private:
    virtual void on_connect(int rc);
    virtual void on_message(const struct mosquitto_message * message);

    /** qos */
    int m_qos;

    /** base topic */
    std::string m_baseTopic;

    /** subscribe topic */
    std::string m_subscribeTopic;

    /** map of payload per topic, to detect changes */
    std::map<std::string, std::string> m_topicPayloads;

    /** mutex to access m_topicPayloads */
    mutable std::mutex m_topicPayloadsMutex;
};


/** singleton */
MqttClient * & mqttClient();
