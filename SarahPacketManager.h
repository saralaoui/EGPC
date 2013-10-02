#include "all-include.h"

#ifndef SARAH_PACKET_MANAGER_H
#define SARAH_PACKET_MANAGER_H
class SarahPacketListener : public osc::OscPacketListener {
protected:

    void ProcessMessage( const osc::ReceivedMessage& m, 
				const IpEndpointName& remoteEndpoint );
};
#endif
