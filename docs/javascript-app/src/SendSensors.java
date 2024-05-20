// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


import com.microsoft.azure.sdk.iot.device.*;
import com.microsoft.azure.sdk.iot.device.exceptions.IotHubClientException;
import com.microsoft.azure.sdk.iot.device.transport.IotHubConnectionStatus;
import lombok.NonNull;
import lombok.SneakyThrows;
import lombok.extern.slf4j.Slf4j;

import java.io.IOException;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.List;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.time.ZoneId;
import java.time.LocalDate;
import java.time.LocalTime;
import java.time.ZonedDateTime;

/** Sends a number of event messages to an IoT Hub. */
public class SendSensors
{
    // The maximum amount of time to wait for a message to be sent. Typically, this operation finishes in under a second.
    private static final int D2C_MESSAGE_TIMEOUT_MILLISECONDS = 15000;

	public static double light1;
	public static double temp1;
	public static double humid1;
	public static double Voc1;
	public static double Nox1;
	public static double O21;
	public static double NDIR1;
	public static double S3021;
	public static double S4021;
	public static double S5021;
	public static double S6021;
	public static double S7021;
	public static double S8021;
	public static double VocB1;
	public static int servo1;

	public static double light2;
	public static double temp2;
	public static double humid2;
	public static double Voc2;
	public static double Nox2;
	public static double O22;
	public static double NDIR2;
	public static double S3022;
	public static double S4022;
	public static double S5022;
	public static double S6022;
	public static double S7022;
	public static double S8022;
	public static double VocB2;
	public static int servo2;

	public static double light3;
	public static double temp3;
	public static double humid3;
	public static double Voc3;
	public static double Nox3;
	public static double O23;
	public static double NDIR3;
	public static double S3023;
	public static double S4023;
	public static double S5023;
	public static double S6023;
	public static double S7023;
	public static double S8023;
	public static double VocB3;
	public static int servo3;

/*
	public static double alight1;
	public static double atemp1;
	public static double ahumid1;
	public static double aVoc1;
	public static double aNox1;
	public static double aO21;
	public static double aNDIR1;
	public static double aS3021;
	public static double aS4021;
	public static double aS5021;
	public static double aS6021;
	public static double aS7021;
	public static double aS8021;
	public static double aVocB1;

	public static double alight2;
	public static double atemp2;
	public static double ahumid2;
	public static double aVoc2;
	public static double aNox2;
	public static double aO22;
	public static double aNDIR2;
	public static double aS3022;
	public static double aS4022;
	public static double aS5022;
	public static double aS6022;
	public static double aS7022;
	public static double aS8022;
	public static double aVocB2;

	public static double alight3;
	public static double atemp3;
	public static double ahumid3;
	public static double aVoc3;
	public static double aNox3;
	public static double aO23;
	public static double aNDIR3;
	public static double aS3023;
	public static double aS4023;
	public static double aS5023;
	public static double aS6023;
	public static double aS7023;
	public static double aS8023;
	public static double aVocB3;
	public static int cnt;
*/

	public static double LAT,LON;

	public static boolean active;


    protected static class IotHubConnectionStatusChangeCallbackLogger implements IotHubConnectionStatusChangeCallback
    {
        @Override
        public void onStatusChanged(ConnectionStatusChangeContext connectionStatusChangeContext)
        {
            IotHubConnectionStatus status = connectionStatusChangeContext.getNewStatus();
            IotHubConnectionStatusChangeReason statusChangeReason = connectionStatusChangeContext.getNewStatusReason();
            Throwable throwable = connectionStatusChangeContext.getCause();

            System.out.println();
            System.out.println("CONNECTION STATUS UPDATE: " + status);
            System.out.println("CONNECTION STATUS REASON: " + statusChangeReason);
            System.out.println("CONNECTION STATUS THROWABLE: " + (throwable == null ? "null" : throwable.getMessage()));
            System.out.println();

            if (throwable != null)
            {
                throwable.printStackTrace();
            }

            if (status == IotHubConnectionStatus.DISCONNECTED)
            {
                System.out.println("The connection was lost, and is not being re-established." +
                    " Look at provided exception for how to resolve this issue." +
                    " Cannot send messages until this issue is resolved, and you manually re-open the device client");
            }
            else if (status == IotHubConnectionStatus.DISCONNECTED_RETRYING)
            {
                System.out.println("The connection was lost, but is being re-established." +
                    " Can still send messages, but they won't be sent until the connection is re-established");
            }
            else if (status == IotHubConnectionStatus.CONNECTED)
            {
                System.out.println("The connection was successfully established. Can send messages.");
            }
        }
    }

/*
	private static void clear_acc()
	{
	cnt=0;

	alight1=0;
	atemp1=0;
	ahumid1=0;
	aVoc1=0;
	aNox1=0;
	aO21=0;
	aNDIR1=0;
	aS3021=0;
	aS4021=0;
	aS5021=0;
	aS6021=0;
	aS7021=0;
	aS8021=0;
	aVocB1=0;

	alight2=0;
	atemp2=0;
	ahumid2=0;
	aVoc2=0;
	aNox2=0;
	aO22=0;
	aNDIR2=0;
	aS3022=0;
	aS4022=0;
	aS5022=0;
	aS6022=0;
	aS7022=0;
	aS8022=0;
	aVocB2=0;

	alight3=0;
	atemp3=0;
	ahumid3=0;
	aVoc3=0;
	aNox3=0;
	aO23=0;
	aNDIR3=0;
	aS3023=0;
	aS4023=0;
	aS5023=0;
	aS6023=0;
	aS7023=0;
	aS8023=0;
	aVocB3=0;

	}

*/

    /**
     * Sends a number of messages to an IoT or Edge Hub. Default protocol is to
     * use MQTT transport.
     *
     */
    public static void connect()
    {

        System.out.println("Starting...");

        String connString = "HostName=fumIOTHub.azure-devices.net;DeviceId=fumdevice;SharedAccessKey=ysfkNNMH/LdMqowlpxwjtO0OK0EUlGuL/daUuSqmfFo="; // params.getConnectionString();

        DeviceClient client = new DeviceClient(connString, IotHubClientProtocol.MQTT);

        System.out.println("Successfully created an IoT Hub client.");

        client.setConnectionStatusChangeCallback(new IotHubConnectionStatusChangeCallbackLogger(), new Object());

		active=false;
		//clear_acc();

        new Thread(new Runnable() {
            @SneakyThrows({InterruptedException.class, IotHubClientException.class})
            @Override
            public void run()
            {


                while (true)
                {

				 if(active) // || cnt > 0)
				 {

//				  if(cnt >= 6)
				  {

/*
	alight1/=cnt;
	atemp1/=cnt;
	ahumid1/=cnt;
	aVoc1/=cnt;
	aNox1/=cnt;
	aO21/=cnt;
	aNDIR1/=cnt;
	aS3021/=cnt;
	aS4021/=cnt;
	aS5021/=cnt;
	aS6021/=cnt;
	aS7021/=cnt;
	aS8021/=cnt;
	aVocB1/=cnt;

	alight2/=cnt;
	atemp2/=cnt;
	ahumid2/=cnt;
	aVoc2/=cnt;
	aNox2/=cnt;
	aO22/=cnt;
	aNDIR2/=cnt;
	aS3022/=cnt;
	aS4022/=cnt;
	aS5022/=cnt;
	aS6022/=cnt;
	aS7022/=cnt;
	aS8022/=cnt;
	aVocB2/=cnt;

	alight3/=cnt;
	atemp3/=cnt;
	ahumid3/=cnt;
	aVoc3/=cnt;
	aNox3/=cnt;
	aO23/=cnt;
	aNDIR3/=cnt;
	aS3023/=cnt;
	aS4023/=cnt;
	aS5023/=cnt;
	aS6023/=cnt;
	aS7023/=cnt;
	aS8023/=cnt;
	aVocB3/=cnt;
*/

				client.open(true);

				System.out.println("Opened connection to IoT Hub.");

				System.out.println("Sending the following event messages:");

				//String currentTime = "\""+java.time.Clock.systemUTC().instant()+"\"";

				//String currentTime = "\""+ZonedDateTime.of(LocalDate.now(), LocalTime.now(), ZoneId.systemDefault())+"\"";

				String currentTime = "\""+java.time.LocalDateTime.now()+"\"";

				String msgStr = "{\"S302\":"+ S3021
				            					+",\"S402\":"+ S4021
				            					+",\"S502\":"+ S5021
				            					+",\"S602\":"+ S6021
				            					+",\"S702\":"+ S7021
				             					+",\"S802\":"+ S8021
				            					+",\"Temp\":"+ temp1
				            					+",\"NDIR\":"+ NDIR1
				             					+",\"Nox\":"+ Nox1
				            					+",\"Voc\":"+ Voc1
				             					+",\"Voc2\":"+ VocB1
				             					+",\"O2\":"+ O21
				             					+",\"Humid\":"+ humid1
				             					+",\"Light\":"+ light1
				             					+",\"GPSX\":"+ LON
				            					+",\"GPSY\":"+ LAT
				            					+",\"Servo\":"+ servo1
				            					+",\"PostedTime\":"+ currentTime
				            					+",\"DeviceName\": \"Unit1\""
				            					+"}";

				            Message msg = new Message(msgStr);
				            msg.setContentType("application/json");
				            msg.setMessageId(java.util.UUID.randomUUID().toString());
				            System.out.println("MSG="+msgStr);


                client.sendEvent(msg, D2C_MESSAGE_TIMEOUT_MILLISECONDS);

				String msgStr2 = "{\"S302\":"+ S3022
				            					+",\"S402\":"+ S4022
				            					+",\"S502\":"+ S5022
				            					+",\"S602\":"+ S6022
				            					+",\"S702\":"+ S7022
				             					+",\"S802\":"+ S8022
				            					+",\"Temp\":"+ temp2
				             					+",\"NDIR\":"+ NDIR2
				             					+",\"Nox\":"+ Nox2
				            					+",\"Voc\":"+ Voc2
				             					+",\"Voc2\":"+ VocB2
				             					+",\"O2\":"+ O22
				             					+",\"Humid\":"+ humid2
				             					+",\"Light\":"+ light2
				             					+",\"GPSX\":"+ LON
				            					+",\"GPSY\":"+ LAT
				            					+",\"Servo\":"+ servo2
				            					+",\"PostedTime\":"+ currentTime
				            					+",\"DeviceName\": \"Unit2\""
				            					+"}";

				            Message msg2 = new Message(msgStr2);
				            msg2.setContentType("application/json");
				            msg2.setMessageId(java.util.UUID.randomUUID().toString());
				            System.out.println("MSG2="+msgStr2);


                client.sendEvent(msg2, D2C_MESSAGE_TIMEOUT_MILLISECONDS);

				String msgStr3 = "{\"S302\":"+ S3023
				            					+",\"S402\":"+ S4023
				            					+",\"S502\":"+ S5023
				            					+",\"S602\":"+ S6023
				            					+",\"S702\":"+ S7023
				             					+",\"S802\":"+ S8023
				            					+",\"Temp\":"+ temp3
				            					+",\"NDIR\":"+ NDIR3
				             					+",\"Nox\":"+ Nox3
				            					+",\"Voc\":"+ Voc3
				             					+",\"Voc2\":"+ VocB3
				             					+",\"O2\":"+ O23
				             					+",\"Humid\":"+ humid3
				             					+",\"Light\":"+ light3
				             					+",\"GPSX\":"+ LON
				            					+",\"GPSY\":"+ LAT
				            					+",\"Servo\":"+ servo3
				            					+",\"PostedTime\":"+ currentTime
				            					+",\"DeviceName\": \"Unit3\""
				            					+"}";

				            Message msg3 = new Message(msgStr3);
				            msg3.setContentType("application/json");
				            msg3.setMessageId(java.util.UUID.randomUUID().toString());
				            System.out.println("MSG3="+msgStr2);


                client.sendEvent(msg3, D2C_MESSAGE_TIMEOUT_MILLISECONDS);

                System.out.println("**Successfully sent the messages**");

                client.close();

                //clear_acc();
				}
			  }
			  else
			  {
					System.out.println("**Azure logging inactive..sleeping**");
			  }

			  if(!active /*&& cnt==0*/)
			  {
				  //clear_acc();

				  Thread.sleep(1000);
			  }
			  else
			  {
					Thread.sleep(60 * 1000);

/*
					cnt++;

	alight1+=light1;
	atemp1+=temp1;
	ahumid1+=humid1;
	aVoc1+=Voc1;
	aNox1+=Nox1;
	aO21+=O21;
	aNDIR1+=NDIR1;
	aS3021+=S3021;
	aS4021+=S4021;
	aS5021+=S5021;
	aS6021+=S6021;
	aS7021+=S7021;
	aS8021+=S8021;
	aVocB1+=VocB1;

	alight2+=light2;
	atemp2+=temp2;
	ahumid2+=humid2;
	aVoc2+=Voc2;
	aNox2+=Nox2;
	aO22+=O22;
	aNDIR2+=NDIR2;
	aS3022+=S3022;
	aS4022+=S4022;
	aS5022+=S5022;
	aS6022+=S6022;
	aS7022+=S7022;
	aS8022+=S8022;
	aVocB2+=VocB2;

	alight3+=light3;
	atemp3+=temp3;
	ahumid3+=humid3;
	aVoc3+=Voc3;
	aNox3+=Nox3;
	aO23+=O23;
	aNDIR3+=NDIR3;
	aS3023+=S3023;
	aS4023+=S4023;
	aS5023+=S5023;
	aS6023+=S6023;
	aS7023+=S7023;
	aS8023+=S8023;
	aVocB3+=VocB3;

System.out.println("SUM "+cnt);

*/

			  }

				} // while
			}// run
        }).start(); // thread


	}

}