
//  Copyright (c) 2003-2021 Xsens Technologies B.V. or subsidiaries worldwide.
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//  1.	Redistributions of source code must retain the above copyright notice,
//  	this list of conditions, and the following disclaimer.
//  
//  2.	Redistributions in binary form must reproduce the above copyright notice,
//  	this list of conditions, and the following disclaimer in the documentation
//  	and/or other materials provided with the distribution.
//  
//  3.	Neither the names of the copyright holders nor the names of their contributors
//  	may be used to endorse or promote products derived from this software without
//  	specific prior written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
//  THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
//  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR
//  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.THE LAWS OF THE NETHERLANDS 
//  SHALL BE EXCLUSIVELY APPLICABLE AND ANY DISPUTES SHALL BE FINALLY SETTLED UNDER THE RULES 
//  OF ARBITRATION OF THE INTERNATIONAL CHAMBER OF COMMERCE IN THE HAGUE BY ONE OR MORE 
//  ARBITRATORS APPOINTED IN ACCORDANCE WITH SAID RULES.
//  

//--------------------------------------------------------------------------------
// Public Xsens device API C++ example MTi receive data.
//--------------------------------------------------------------------------------
#include <xscontroller/xscontrol_def.h>
#include <xscontroller/xsdevice_def.h>
#include <xscontroller/xsscanner.h>
#include <xstypes/xsoutputconfigurationarray.h>
#include <xstypes/xsdatapacket.h>
#include <xstypes/xstime.h>
#include <xscommon/xsens_mutex.h>

#include <iostream>
#include <iomanip>
#include <list>
#include <string>
#include <cmath>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int _kbhit();

Journaller* gJournal = 0;

using namespace std;

class CallbackHandler : public XsCallback
{
public:
	CallbackHandler(size_t maxBufferSize = 5)
		: m_maxNumberOfPacketsInBuffer(maxBufferSize)
		, m_numberOfPacketsInBuffer(0)
	{
	}

	virtual ~CallbackHandler() throw()
	{
	}

	bool packetAvailable() const
	{
		xsens::Lock locky(&m_mutex);
		return m_numberOfPacketsInBuffer > 0;
	}

	XsDataPacket getNextPacket()
	{
		assert(packetAvailable());
		xsens::Lock locky(&m_mutex);
		XsDataPacket oldestPacket(m_packetBuffer.front());
		m_packetBuffer.pop_front();
		--m_numberOfPacketsInBuffer;
		return oldestPacket;
	}

protected:
	void onLiveDataAvailable(XsDevice*, const XsDataPacket* packet) override
	{
		xsens::Lock locky(&m_mutex);
		assert(packet != 0);
		while (m_numberOfPacketsInBuffer >= m_maxNumberOfPacketsInBuffer)
			(void)getNextPacket();

		m_packetBuffer.push_back(*packet);
		++m_numberOfPacketsInBuffer;
		assert(m_numberOfPacketsInBuffer <= m_maxNumberOfPacketsInBuffer);
	}
private:
	mutable xsens::Mutex m_mutex;

	size_t m_maxNumberOfPacketsInBuffer;
	size_t m_numberOfPacketsInBuffer;
	list<XsDataPacket> m_packetBuffer;
};

//--------------------------------------------------------------------------------
int main(void)
{
	cout << "Opening Socket" << endl;
	int ss = socket(AF_INET, SOCK_DGRAM, 0);
	assert(ss >= 0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6543);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	assert(r != -1);
	char cbuff[1000];
	
    int count = 0; //csomag szamlalao, debug

	cout << "Creating XsControl object..." << endl;
	XsControl* control = XsControl::construct();
	assert(control != 0);

	// Lambda function for error handling
	auto handleError = [=](string errorString)
	{
		control->destruct();
		cout << errorString << endl;
		cout << "Press [ENTER] to continue." << endl;
		cin.get();
		return -1;
	};

	cout << "Scanning for devices..." << endl;
	XsPortInfoArray portInfoArray = XsScanner::scanPorts();

	// Find an MTi device
	XsPortInfo mtPort;
	for (auto const &portInfo : portInfoArray)
	{
		if (portInfo.deviceId().isMti() || portInfo.deviceId().isMtig())
		{
			mtPort = portInfo;
			break;
		}
	}

	if (mtPort.empty())
		return handleError("No MTi device found. Aborting.");

	cout << "Found a device with ID: " << mtPort.deviceId().toString().toStdString() << " @ port: " << mtPort.portName().toStdString() << ", baudrate: " << mtPort.baudrate() << endl;

	cout << "Opening port..." << endl;
	if (!control->openPort(mtPort.portName().toStdString(), mtPort.baudrate()))
		return handleError("Could not open port. Aborting.");

	// Get the device object
	XsDevice* device = control->device(mtPort.deviceId());
	assert(device != 0);

	cout << "Device: " << device->productCode().toStdString() << ", with ID: " << device->deviceId().toString() << " opened." << endl;

	// Create and attach callback handler to device
	CallbackHandler callback;
	device->addCallbackHandler(&callback);

	// Put the device into configuration mode before configuring the device
	cout << "Putting device into configuration mode..." << endl;
	if (!device->gotoConfig())
		return handleError("Could not put device into configuration mode. Aborting.");

	cout << "Configuring the device..." << endl;

	// Important for Public XDA!
	// Call this function if you want to record a mtb file:
	//device->readEmtsAndDeviceConfiguration();

	XsOutputConfigurationArray configArray;
	configArray.push_back(XsOutputConfiguration(XDI_PacketCounter, 0));
	configArray.push_back(XsOutputConfiguration(XDI_SampleTimeFine, 0));
	//GNSS device, itt mondjuk meg mit kuldjon
//	configArray.push_back(XsOutputConfiguration(XDI_Quaternion, 25));
	configArray.push_back(XsOutputConfiguration(XDI_EulerAngles, 100));
	configArray.push_back(XsOutputConfiguration(XDI_RateOfTurn , 100));
	//configArray.push_back(XsOutputConfiguration(XDI_DeltaQ, 25));
	configArray.push_back(XsOutputConfiguration(XDI_GnssPvtData, 4));
	configArray.push_back(XsOutputConfiguration(XDI_VelocityXYZ, 100));

	if (!device->setOutputConfiguration(configArray))
		return handleError("Could not configure MTi device. Aborting.");

    // 11 - general, 12 - nobaro, 13 - generalmag
    if(!device->setOnboardFilterProfile(13)){
		cout << "************* ez nem jott ossze ***********************";
	}
	// az U-blox adatalp szerint ez adja a legjobb pontossagot.
	// az 
	if(!device->setGnssPlatform(XGP_Pedestrian))
		cout << "setplatform nem ok";
	if(!device->setDeviceOptionFlags(XDOF_EnableOrientationSmoother,
		XDOF_EnableBeidou | XDOF_DisableGps | XDOF_EnableAhs | XDOF_EnableInrunCompassCalibration))
		cout << "setDeviceOptionFlags nem ok";

	cout << "Putting device into measurement mode..." << endl;
	if (!device->gotoMeasurement())
		return handleError("Could not put device into measurement mode. Aborting.");

	cout << "\nMegy a vonat..." << endl;
	cout << string(79, '-') << endl;
    float roll, pitch, yaw;
    float spX, spY, spW; // INS speeds X, Y es Z iranyu szogsebesseg

	while (1)
	{
		if (callback.packetAvailable())
		{
			//cout << setw(5) << fixed << setprecision(2);

            bool sendpacket = false;
            

			// Retrieve a packet
			XsDataPacket packet = callback.getNextPacket();

			if (packet.containsOrientation())
			{
				XsEuler euler = packet.orientationEuler();
				//cout << "\r |Roll:" << euler.roll()
				//	<< ", Pitch:" << euler.pitch()
				//	<< ", Yaw:" << euler.yaw();

                roll = euler.roll(); pitch = euler.pitch(); yaw = euler.yaw();
                sendpacket = true;
                count++;
			}

			if (packet.containsCalibratedGyroscopeData())
			{
				XsVector vec = packet.calibratedGyroscopeData();
				
				//cout << " Gyr:" << vec[0];
				//cout << ":" << vec[1] << ":" << vec[2] << "  ";

                spW = vec[2];
                sendpacket = true;
			}
			
			if(packet.containsRawGnssPvtData())
			{
				XsRawGnssPvtData pvt = packet.rawGnssPvtData();
				//cout << " |Acc: " << pvt.m_hAcc;
				//cout << " Sat#: " << (int)pvt.m_numSv;
				//cout << " Speed E: " << pvt.m_velE;
				//cout << " N: " << pvt.m_velN;
				//cout << " SpdAcc:" << pvt.m_sAcc << " fixType:" << (int)pvt.m_fixType << "  ";
                //be is kell forgatni
                float f = yaw * -M_PI/180;
                float vx = pvt.m_velE/1000.0*cos(f) - pvt.m_velN/1000.0*sin(f);
                float vy = pvt.m_velE/1000.0*sin(f) + pvt.m_velN/1000.0*cos(f);

				//create json format
				int l = sprintf(cbuff, "{\"gps\": 1, \"acc\": %d, \
					\"sat\": %d, \"spd_e\": %d, \"spd_n\": %d, \"Vgps_vec\":[%f, %f, %f]}",
					pvt.m_hAcc, (int)pvt.m_numSv, pvt.m_velE, pvt.m_velN, vx, vy, spW);
				sendto(ss, cbuff, l, 0, (struct sockaddr*)&addr, sizeof(addr)); 
			}

			if (packet.containsVelocity())
			{
				XsVector vel = packet.velocity(XDI_CoordSysEnu);
				//cout << "INS speed: |E:" << vel[0]
				//	<< ", N:" << vel[1]
				//	<< ", U:" << vel[2];

                sendpacket = true;
                // ezt meg be is kell forgatni mert ENU rendszerben van
                float f = yaw * -M_PI/180;
                spX = vel[0]*cos(f) - vel[1]*sin(f);
                spY = vel[0]*sin(f) + vel[1]*cos(f);
			} else {
                spX = spY = 0.0;
            }

            if(sendpacket){
				//create json format
                // roll, pitch, yaw: szogek
                // Vvec[] ez a szimulacioban hasznalt sebesseg vektor. a [2] tag a Z szogsebesseg
				int l = sprintf(cbuff, "{\"ang\": 1, \"roll\": %f, \
				\"pitch\": %f, \"yaw\": %f, \"Vvec\": [%f, %f, %f]}" ,
                roll, pitch, yaw, spX, spY, spW);
				sendto(ss, cbuff, l, 0, (struct sockaddr*)&addr, sizeof(addr)); 
            }

            // KI KELL SZEDNI A KIIRATAST, MERT NEM BIRJA!!
            if(count % 100 == 0){
                cout << "\r Count: " << count << "  ";
			    cout << flush;
            }
		} else XsTime::msleep(1);

        XsTime::msleep(0); //ez talan kell valami threading miatt?
		if(_kbhit()) break;
	}
	cout << "\n" << string(79, '-') << "\n";
	cout << endl;

	cout << "Closing port..." << endl;
	control->closePort(mtPort.portName().toStdString());

	cout << "Freeing XsControl object..." << endl;
	control->destruct();

	cout << "Successful exit." << endl;

	cout << "Press [ENTER] to continue." << endl;
	cin.get();

	return 0;
}
