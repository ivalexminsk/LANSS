import java.io.*;
import java.lang.reflect.Array;
import java.math.BigInteger;
import java.net.*;
import java.text.SimpleDateFormat;
import java.util.*;

class UDPServer
{

    public static int unpackDataToDictionary(byte[] data, Map<BigInteger, byte[]> dictionary) {
        int isLast = data[0];

        if (isLast == 1) {
            return isLast;
        }

        byte[] byteIndex = {data[3], data[2], data[1]};
        BigInteger index = new BigInteger(byteIndex);

        byte[] fileData = data;

        for (int i = 0; i < 4; i++) {
            fileData = removeElement(fileData, 0);
        }

        byte slashN = "\n".getBytes()[0];
        for (int i = fileData.length - 1; i >= 0; i--) {

            if (fileData[i] == slashN) {
                if (fileData[i - 1] == slashN && fileData[i - 2] == slashN && fileData[i - 3] == slashN) {
                    for (int j = fileData.length - 1; j >= i - 3; j--) {
                        fileData = removeElement(fileData, fileData.length - 1);
                    }
                    break;
                }
            }
        }

        dictionary.put(index, fileData);

        return isLast;
    }

    public static int unpackLast(byte[] data) {
        data = removeElement(data, 0);

        byte slashN = "\n".getBytes()[0];
        for (int i = data.length - 1; i >= 0; i--) {

            if (data[i] == slashN) {
                if (data[i - 1] == slashN && data[i - 2] == slashN && data[i - 3] == slashN) {
                    for (int j = data.length - 1; j >= i - 3; j--) {
                        data = removeElement(data, data.length - 1);
                    }
                    break;
                }
            }
        }

        return (new BigInteger(data)).intValue();
    }

    public static void main(String args[]) throws Exception {
        int packetSize = 600;

        DatagramSocket serverSocket = new DatagramSocket(9876);
        byte[] receiveData = new byte[packetSize];
        byte[] sendData = new byte[packetSize];

        boolean connectionLost = false;

        while (true)
        {
            receiveData = new byte[packetSize];
            DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
            // Данные от клиента будут ожидаться в течение 10 секунд
            serverSocket.setSoTimeout(10000);
            try {
                serverSocket.receive(receivePacket);

                if (connectionLost == true) {
                    connectionLost = false;
                    System.out.println("CONNECTION WAS RESTORED");
                }

                InetAddress IPAddress = receivePacket.getAddress();
                int port = receivePacket.getPort();

                String sentence = new String(receivePacket.getData()).replace("\u0000", "");
                sentence = sentence.replace("\n", "");

                if (sentence.contentEquals("TIME")) {
                    sendData = new byte[packetSize];

                    String capitalizedSentence = getTime();
                    sendData = capitalizedSentence.getBytes();

                    DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, port);
                    serverSocket.send(sendPacket);
                } else if (sentence.contentEquals("EXIT")) {
                    break;
                } else if (sentence.contains("PREPARE_FOR_FILE_RECEIVING")) {
                    List<String> result = Arrays.asList(sentence.split(" "));
                    System.out.println("RECEIVING FILE: " + result.get(1));

                    byte[] response = "GIVE_ME_THIS_SHIT".getBytes();
                    DatagramPacket sendPacket = new DatagramPacket(response, response.length, IPAddress, port);
                    serverSocket.send(sendPacket);

                    List<byte[]> rawPackets = new ArrayList<byte[]>();

                    int lastIndex = 0;

                    while (true) {
                        byte[] data = getData(serverSocket);

                        if (data[0] == 1) {
                            lastIndex = unpackLast(data);
                            break;
                        }

                        rawPackets.add(data);
                    }

                    Map<BigInteger, byte[]> packets = new HashMap<BigInteger, byte[]>();
                    for (int i = 0; i < rawPackets.size(); i++) {
                        unpackDataToDictionary(rawPackets.get(i), packets);
                    }

                    File f = new File("." + File.separator + result.get(1));
                    FileOutputStream outputStream = new FileOutputStream(result.get(1));
                    //File f = new File("ServerFolder/" + result.get(1));
                    //FileOutputStream outputStream = new FileOutputStream("ServerFolder/" + result.get(1));

                    System.out.println("Size = " + lastIndex);

                    for (int i = 0; i < lastIndex; i++) {
                        byte[] d = packets.get(BigInteger.valueOf(i));

                        if (d != null) {
                            outputStream.write(d);
                        } else {
                            //System.out.println("Blya(");
                        }
                    }
                    System.out.println("Real size = " + packets.size());
                    outputStream.close();
                } else {
                    sendData = new byte[packetSize];
                    sendData = sentence.getBytes();

                    DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, port);
                    serverSocket.send(sendPacket);
                }






            } catch (SocketTimeoutException e) {
                System.out.println("Timeout reached! No client connection!");
                connectionLost = true;
            }

        }

        serverSocket.close();
    }

    public static String getTime() {
        Calendar cal = Calendar.getInstance();
        SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
        return sdf.format(cal.getTime());
    }

    private static byte[] insertElement(byte original[], byte element, int index) {
        int length = original.length;
        byte destination[] = new byte[length + 1];
        System.arraycopy(original, 0, destination, 0, index);
        destination[index] = element;
        System.arraycopy(original, index, destination, index + 1, length - index);
        return destination;
    }

    public static byte[] removeElement(byte[] arr, int index) {
        // Destination array
        byte[] arrOut = new byte[arr.length - 1];
        int remainingElements = arr.length - ( index + 1 );
        // copying elements that come before the index that has to be removed
        System.arraycopy(arr, 0, arrOut, 0, index);
        // copying elements that come after the index that has to be removed
        System.arraycopy(arr, index + 1, arrOut, index, remainingElements);
        return arrOut;
    }

    // Принять пакет
    public static byte[] getData(DatagramSocket socket) {
        byte[] receiveData = new byte[133];

        DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);

        try {
            // Данные от клиента будут ожидаться в течение 5 секунд
            socket.setSoTimeout(2000);
        } catch (SocketException e) {
            e.printStackTrace();
        }
        try {
            socket.receive(receivePacket);
            //System.out.println("Got packet!");
            return receivePacket.getData();
        }
        catch (SocketTimeoutException e) {
            System.out.println("Timeout reached! No client connection!");
        } catch (IOException e) {
            e.printStackTrace();
        }

        return receiveData;
    }
}