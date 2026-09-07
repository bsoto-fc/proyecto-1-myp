package mx.unam.ciencias.myp.sockets

import java.net.Socket
import java.net.ConnectException
import java.net.SocketTimeoutException
import java.io.IOException


// Obtenido de https://kotlin.unisbadri.com/en/advanced/socket/#tcp-socket--client
fun socket(ip: String, port: Int){
    try {
        Socket(ip, port).use { socket ->
            // Set a timeout — don't let the socket wait forever
            socket.soTimeout = 5000  // 5 second read timeout
            
            val writer = socket.getOutputStream().bufferedWriter()
            val reader = socket.getInputStream().bufferedReader()
            
            // Send a message
            val message = "Hello from client!"
            writer.write("$message\n")
            writer.flush()
            println("Sent: $message")
            
            // Read the response
            val response = reader.readLine()
            println("Server response: $response")
        }
    } catch (e: ConnectException) {
        println("Failed to connect to $ip:$port — is the server running?")
    } catch (e: SocketTimeoutException) {
        println("Timeout: the server didn't respond within 5 seconds")
    } catch (e: IOException) {
        println("Network error: ${e.message}")
    }
}
