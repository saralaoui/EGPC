#include "all-include.h"
#include "SarahPacketManager.h"


/*class SarahPacketListener : public osc::OscPacketListener {
protected:
*/

void SarahPacketListener::ProcessMessage( const osc::ReceivedMessage& m, 
				const IpEndpointName& remoteEndpoint )
    {
		
        try{
            // parsing single messages. osc::OsckPacketListener
            // handles the bundle traversal.

            if( strcmp( m.AddressPattern(), "/indice" ) == 0 ){
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                float a0;
                args >> a0 >> osc::EndMessage;
				indice = a0;
			}
			
			if( strcmp( m.AddressPattern(), "/raideur" ) == 0 ){
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                float a1;
                args >> a1 >> osc::EndMessage;
				raideur[(int)indice] = a1;
			}
			if( strcmp( m.AddressPattern(), "/longueur" ) == 0 ){
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                float a2;
                args >> a2 >> osc::EndMessage;
				elongation[(int)indice] = a2;
			}
			
			if( strcmp( m.AddressPattern(), "/mu" ) == 0 ){
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                float a4;
                args >> a4 >> osc::EndMessage;
				mu = a4;
			}
			
			if( strcmp( m.AddressPattern(), "/MassePlus" ) == 0 ){
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                float a5;
                args >> a5 >> osc::EndMessage;
				MassePlus = a5;
			}
			
			
			
			if( strcmp( m.AddressPattern(), "/NBedges" ) == 0 ){
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                float a9;
                args >> a9 >> osc::EndMessage;
				NB_EDGES = a9;
			}
			
			if( strcmp( m.AddressPattern(), "/NBmasse" ) == 0 ){
                osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
                float a10;
                args >> a10 >> osc::EndMessage;
				NB_MASSES = a10;
			}

			
			else {
				osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
				std::cout << "received message: " <<  m.AddressPattern() << endl;
			}
        } catch( osc::Exception& e ){
            // any parsing errors such as unexpected argument types, or 
            // missing arguments get thrown as exceptions.
            std::cout << "error while parsing message: "
                << m.AddressPattern() <<  ": " << e.what() << "\n";

        }
    }



