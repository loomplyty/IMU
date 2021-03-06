/* Copyright (c) Xsens Technologies B.V., 2006-2012. All rights reserved.

	  This source code is provided under the MT SDK Software License Agreement
and is intended for use only by Xsens Technologies BV and
	   those that have explicit written permission to use it from
	   Xsens Technologies BV.

	  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	   PARTICULAR PURPOSE.
 */

#include <xsens/xsportinfoarray.h>
#include <xsens/xsdatapacket.h>
#include <xsens/xstime.h>
#include <xcommunication/legacydatapacket.h>
#include <xcommunication/int_xsdatapacket.h>
#include <xcommunication/enumerateusbdevices.h>
#include <xcommunication/mtwsdidata.h>

#include "deviceclass.h"

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>


#ifdef __GNUC__
#include "conio.h" // for non ANSI _kbhit() and _getch()
#else
#include <conio.h>
#endif

int main(int argc, char* argv[])
{
	DeviceClass device;

	try
	{
		// Scan for connected USB devices
		std::cout << "Scanning for USB devices..." << std::endl;
		XsPortInfoArray portInfoArray;
		xsEnumerateUsbDevices(portInfoArray);
		if (!portInfoArray.size())
		{
			std::string portNumber="/dev/ttyUSB0";
			int baudRate=921600;

			std::cout << "No USB Motion Tracker found." << std::endl << std::endl
					<< "COM port name set as "<<portNumber <<std::endl
					<< "Baud rate set as "<<baudRate<<std::endl<<std::endl;

			XsPortInfo portInfo(portNumber, XsBaud::numericToRate(baudRate));
			portInfoArray.push_back(portInfo);
		}

		// Use the first detected device
		XsPortInfo mtPort = portInfoArray.at(0);

		// Open the port with the detected device
		std::cout << "Opening port..." << std::endl;
		if (!device.openPort(mtPort))
			throw std::runtime_error("Could not open port. Aborting.");

		// Put the device in configuration mode
		std::cout << "Putting device into configuration mode..." << std::endl;
		if (!device.gotoConfig()) // Put the device into configuration mode before configuring the device
		{
			throw std::runtime_error("Could not put device into configuration mode. Aborting.");
		}

		// Request the device Id to check the device type
		mtPort.setDeviceId(device.getDeviceId());

		// Check if we have an MTi / MTx / MTmk4 device
		if (!mtPort.deviceId().isMt9c() && !mtPort.deviceId().isMtMk4())
		{
			throw std::runtime_error("No MTi / MTx / MTmk4 device found. Aborting.");
		}
		std::cout << "Found a device with id: " << mtPort.deviceId().toString().toStdString() << " @ port: " << mtPort.portName().toStdString() << ", baudrate: " << mtPort.baudrate() << std::endl;

		try
		{
			// Print information about detected MTi / MTx / MTmk4 device
			std::cout << "Device: " << device.getProductCode().toStdString() << " opened." << std::endl;

			// Configure the device. Note the differences between MTix and MTmk4
			std::cout << "Configuring the device..." << std::endl;
			if (mtPort.deviceId().isMt9c())
			{
				XsOutputMode outputMode = XOM_Orientation; // output orientation data
				XsOutputSettings outputSettings = XOS_OrientationMode_Quaternion; // output orientation data as quaternion

				// set the device configuration
				if (!device.setDeviceMode(outputMode, outputSettings))
				{
					throw std::runtime_error("Could not configure MT device. Aborting.");
				}
			}
			else if (mtPort.deviceId().isMtMk4())
			{
				XsOutputConfiguration quat_Q(XDI_Quaternion, 100);
				XsOutputConfiguration quat_DQ(XDI_DeltaQ, 100);
				XsOutputConfiguration quat_DV(XDI_DeltaV, 100);
				XsOutputConfiguration quat_ACC(XDI_Acceleration, 100);
				XsOutputConfigurationArray configArray;
				configArray.push_back(quat_Q);
				configArray.push_back(quat_DQ);
				configArray.push_back(quat_DV);
				configArray.push_back(quat_ACC);
				if (!device.setOutputConfiguration(configArray))
				{

					throw std::runtime_error("Could not configure MTmk4 device. Aborting.");
				}
			}
			else
			{
				throw std::runtime_error("Unknown device while configuring. Aborting.");
			}

			// Put the device in measurement mode
			std::cout << "Putting device into measurement mode..." << std::endl;
			if (!device.gotoMeasurement())
			{
				throw std::runtime_error("Could not put device into measurement mode. Aborting.");
			}

			std::cout << "\nMain loop (press any key to quit)" << std::endl;
			std::cout << std::string(79, '-') << std::endl;

			XsByteArray data;
			XsMessageArray msgs;
			while (!_kbhit())
			{
				device.readDataToBuffer(data);
				device.processBufferedData(data, msgs);
				for (XsMessageArray::iterator it = msgs.begin(); it != msgs.end(); ++it)
				{
					// Retrieve a packet
					XsDataPacket packet;
					if ((*it).getMessageId() == XMID_MtData) {
						LegacyDataPacket lpacket(1, false);
						lpacket.setMessage((*it));
						lpacket.setXbusSystem(false, false);
						lpacket.setDeviceId(mtPort.deviceId(), 0);
						lpacket.setDataFormat(XOM_Orientation, XOS_OrientationMode_Quaternion,0);	//lint !e534
						XsDataPacket_assignFromXsLegacyDataPacket(&packet, &lpacket, 0);
					}
					else if ((*it).getMessageId() == XMID_MtData2) {
						packet.setMessage((*it));
						packet.setDeviceId(mtPort.deviceId());
					}

					// Get the quaternion data
					bool a0=packet.containsOrientation();
					//std::cout<<"a0:"<<a0;
					XsQuaternion quaternion = packet.orientationQuaternion();
/*
					std::cout << "\r"
						<< "W:" << std::setw(5) << std::fixed << std::setprecision(2) << quaternion.m_w
						<< ",X:" << std::setw(5) << std::fixed << std::setprecision(2) << quaternion.m_x
						<< ",Y:" << std::setw(5) << std::fixed << std::setprecision(2) << quaternion.m_y
						<< ",Z:" << std::setw(5) << std::fixed << std::setprecision(2) << quaternion.m_z
					;
*/
					// Get deltaQ
					bool a1=packet.containsSdiData();
					//std::cout<<"a1:"<<a1;
					XsSdiData deltaParam=packet.sdiData();
					XsQuaternion deltaQ=deltaParam.orientationIncrement();
					XsVector deltaV=deltaParam.velocityIncrement();
/*
					std::cout<<"\r"
						<< "dQ W:" << std::setw(5) << std::fixed << std::setprecision(2) << deltaQ.m_w
						<< ",dQ X:" << std::setw(5) << std::fixed << std::setprecision(2) << deltaQ.m_x
						<< ",dQ Y:" << std::setw(5) << std::fixed << std::setprecision(2) << deltaQ.m_y
						<< ",dQ Z:" << std::setw(5) << std::fixed << std::setprecision(2) << deltaQ.m_z
					;
					/*
					std::cout<<"\r"
						<< "dV X:" << std::setw(5) << std::fixed << std::setprecision(2) << deltaV[0]
						<< ",dV Y:" << std::setw(5) << std::fixed << std::setprecision(2) << deltaV[1]
						<< ",dV Z:" << std::setw(5) << std::fixed << std::setprecision(2) << deltaV[2]
					;
*/
					// Convert packet to euler
					XsEuler euler = packet.orientationEuler();
					double angle[3];
					angle[0]=euler.m_x/180*3.14;
					angle[1]=euler.m_y/180*3.14;
					angle[2]=euler.m_z/180*3.14;
/*
					std::cout<< "\r"
						<< ",Roll:" << std::setw(7) << std::fixed << std::setprecision(2) << euler.m_roll
						<< ",Pitch:" << std::setw(7) << std::fixed << std::setprecision(2) << euler.m_pitch
						<< ",Yaw:" << std::setw(7) << std::fixed << std::setprecision(2) << euler.m_yaw
					;
*/
					// Get angular velocity

					double angularVelocity[3];
					angularVelocity[0] = deltaQ[1] *100 * 2.0; //100 is the configured frequency
					angularVelocity[1] = deltaQ[2] *100 * 2.0;
					angularVelocity[2] = deltaQ[3] *100 * 2.0;
/*
					std::cout
						<< "aVel X:" << std::setw(7) << std::fixed << std::setprecision(2) << angularVelocity[0]
						<< ",aVel Y:" << std::setw(7) << std::fixed << std::setprecision(2) << angularVelocity[1]
						<< ",aVel Z:" << std::setw(7) << std::fixed << std::setprecision(2) << angularVelocity[2]
					;
*/

					// Get acceleration
					bool a2=packet.containsCalibratedAcceleration();
					XsVector acceleration=packet.calibratedAcceleration();
					double linearAcc[3];
					XsVector(acceleration,linearAcc,3);

					std::cout<<"\r"<<"a2:"<<a2<<"  "
							<< "Acc X:" << std::setw(7) << std::fixed << std::setprecision(2) << linearAcc[0]
							<< ",Acc Y:" << std::setw(7) << std::fixed << std::setprecision(2) << linearAcc[1]
							<< ",Acc Z:" << std::setw(7) << std::fixed << std::setprecision(2) << linearAcc[2]
					;

					std::cout << std::flush;
				}

				msgs.clear();
				XsTime::msleep(0);
			}
			_getch();
			std::cout << "\n" << std::string(79, '-') << "\n";
			std::cout << std::endl;
		}
		catch (std::runtime_error const & error)
		{
			std::cout << error.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "An unknown fatal error has occured. Aborting." << std::endl;
		}

		// Close port
		std::cout << "Closing port..." << std::endl;
		device.close();
	}
	catch (std::runtime_error const & error)
	{
		std::cout << error.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "An unknown fatal error has occured. Aborting." << std::endl;
	}

	std::cout << "Successful exit." << std::endl;

	std::cout << "Press [ENTER] to continue." << std::endl; std::cin.get();

	return 0;
}
