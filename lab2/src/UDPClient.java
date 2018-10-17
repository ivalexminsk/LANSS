import java.util.*;

import java.io.*;
import java.math.BigInteger;
import java.net.*;
import java.util.Arrays;

class UDPClient
{
    // Отправить пакет
    public static void sendData(byte[] data, DatagramSocket socket, InetAddress IPAddress, int port) throws Exception {
        byte[] slashN = "\n\n\n\n".getBytes();
        for (int i = 0; i < slashN.length; i++) {
            data = insertElement(data, slashN[i], data.length);
        }

        DatagramPacket sendPacket = new DatagramPacket(data, data.length, IPAddress, port);
        socket.send(sendPacket);
    }

    // Принять пакет
    public static byte[] getData(DatagramSocket socket) {
        byte[] receiveData = new byte[600];

        DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);

        try {
            // Ответ от сервера будет ожидаться в течение 5 секунд
            socket.setSoTimeout(5000);
        } catch (SocketException e) {
            e.printStackTrace();
        }
        try {
            socket.receive(receivePacket);
            return receivePacket.getData();
        }
        catch (SocketTimeoutException e) {
            System.out.println("Timeout reached! No server connection!");
        } catch (IOException e) {
            e.printStackTrace();
        }

        return receiveData;
    }

    // Формируем пакет для отправки. Каждый пакет имеет собственный порядковый номер, который записывается в начале
    public static byte[] createPacket(byte[] data, int index, int lastOne) throws FileNotFoundException {
        byte[] indexData = BigInteger.valueOf(index).toByteArray();

        for (int i = indexData.length - 1, j = 0; i >= 0; i--, j++) {
            data = insertElement(data, indexData[i], j);
        }

        for (int i = indexData.length; i < 3; i++) {
            data = insertElement(data, (byte)0x00, i);
        }

        data = insertElement(data, (byte)lastOne, 0);

        return data;
    }

    public static void unpackDataToDictionary(byte[] data, Map<BigInteger, byte[]> dictionary) {
        byte[] byteIndex = {data[2], data[1], data[0]};
        BigInteger index = new BigInteger(byteIndex);

        byte[] fileData = data;

        for (int i = 0; i < 3; i++) {
            fileData = removeElement(fileData, 0);
        }

        byte slashN = "\n".getBytes()[0];
        for (int i = data.length - 1; i >= 0; i--) {

            if (data[i] == slashN) {
                if (data[i - 1] == slashN && data[i - 2] == slashN && data[i - 3] == slashN) {
                    for (int j = data.length - 1; j >= i - 3; j--) {
                        data = removeElement(data, data.length - 1);
                    }
                    return;
                }
            }
        }

        dictionary.put(index, fileData);
    }

    public static void sendFile(String fileName, DatagramSocket socket, InetAddress IPAddress, int port) throws Exception {
        // Информируем сервер о том, что он должен принять данные от клиента
        sendData(("PREPARE_FOR_FILE_RECEIVING " + fileName).getBytes(), socket, IPAddress, port);
        String response = new String(getData(socket)).replace("\u0000", "");
        if (!response.contains("GIVE_ME_THIS_SHIT")) {
            return;
        }

        FileInputStream inputStream = new FileInputStream("ClientFolder/" + fileName);
        int counter = 0;
        int nRead = 0;
        byte data[] = new byte[120];

        while ((nRead = inputStream.read(data)) != -1) {
            // Создали пакет для отправки
            data = createPacket(data, counter, 0);

            counter += 1;
            sendData(data, socket, IPAddress, port);

            // Почистили буфер
            data = new byte[120];

            if (counter % 10 == 0) {
                Thread.sleep(1);
                //System.out.println("kek");
            }
        }

        byte[] counterBytes = BigInteger.valueOf(counter).toByteArray();
        byte[] lastData = {1};
        for (int i = counterBytes.length - 1; i >= 0; i--) {
            lastData = insertElement(lastData, counterBytes[i], 1);
        }
        sendData(lastData, socket, IPAddress, port);

        System.out.println("Sent all packets) - " + counter);
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

    public static void main(String args[]) throws Exception
    {
        BufferedReader inFromUser = new BufferedReader(new InputStreamReader(System.in));
        DatagramSocket clientSocket = new DatagramSocket();

        // 172.20.10.4
        InetAddress IPAddress = InetAddress.getByName("172.20.10.3");
        int port = 9876;

        while(true) {
            String sentence = inFromUser.readLine();

            List<String> result = Arrays.asList(sentence.split(" "));

            if (result.size() == 2) {
                if (result.get(0).contentEquals("ECHO")) {
                    String sendString = result.get(1);
                    sendString += "\n";

                    // Отправляем данные
                    sendData(sendString.getBytes(), clientSocket, IPAddress, port);
                    // Принимаем данные
                    String modifiedSentence = new String(getData(clientSocket));
                    // Выводим очищенные данные
                    System.out.println(modifiedSentence.replace("\u0000", ""));
                }
                if (result.get(0).contentEquals("UPLOAD")) {
                    try {
                        FileInputStream inputStream = new FileInputStream("ClientFolder/" + result.get(1));
                        System.out.println("START UPLOADING FILE...");

                        sendFile(result.get(1), clientSocket, IPAddress, port);
                    } catch (IOException exc) {
                        System.out.println("FILE NOT FOUND!");
                    }
                }
            } else if (result.get(0).contentEquals("EXIT")) {
                String sendString = result.get(0);
                sendString += "\n";

                // Отправляем данные
                sendData(sendString.getBytes(), clientSocket, IPAddress, port);

                break;
            } else if (result.get(0).contentEquals("TIME")) {
                String sendString = result.get(0);
                sendString += "\n";

                // Отправляем данные
                sendData(sendString.getBytes(), clientSocket, IPAddress, port);
                // Принимаем данные
                String modifiedSentence = new String(getData(clientSocket)).replace("\u0000", "");
                // Выводим очищенные данные
                System.out.println(modifiedSentence);
            } else {
                System.out.println("WRONG COMMAND!");
            }
        }

        clientSocket.close();
    }
}