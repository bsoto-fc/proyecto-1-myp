package mx.unam.ciencias.myp.sockets

import kotlinx.coroutines.*
import java.io.BufferedReader
import java.io.IOException
import java.io.InputStreamReader
import java.io.PrintWriter
import java.net.Socket
import java.net.SocketException
import kotlinx.serialization.SerializationException

import mx.unam.ciencias.myp.messages.*

class SocketClient(
    private val ip: String,
    private val port: Int,
    private val username: String,
    private val onMessageReceived: (String) -> Unit,
    private val onDisconnected: (String) -> Unit,
    private val onConnected: () -> Unit = {}
) {
    private var socket: Socket? = null
    private var writer: PrintWriter? = null
    private var reader: BufferedReader? = null
    
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    
    val isConnected: Boolean
        get() = socket?.isConnected == true && socket?.isClosed == false
    
    fun connect(){
        scope.launch {
            try {
                println("[CLIENT]: Conectandose a $ip:$port")
                val s = Socket(ip, port)
                socket = s
                writer = PrintWriter(s.getOutputStream(), true)
                reader = BufferedReader(InputStreamReader(s.getInputStream()))

                val usernamePetition = json.encodeToString(Message(
                                                               type = "IDENTIFY",
                                                               username = username
                                                           ))
                writer?.println(usernamePetition)
                println("[CLIENT]: Petición de identifiación enviada al servidor.")

                val rawResponse = reader?.readLine()
                println("[CLIENT]: Se obtuvo respuesta $rawResponse")
                if (rawResponse == null) {
                    withContext(Dispatchers.Main){
                        onDisconnected("El servidor cerró la conexión.")
                    }
                    return@launch
                }
                
                val serverResponse = json.decodeFromString<Message>(rawResponse)
                
                when(serverResponse.result){
                    "USER_ALREADY_EXISTS" -> {
                                                 withContext(Dispatchers.Main){
                                                     onDisconnected("Usuario ya existente.")
                                                 }
                                                 return@launch
                                             }
                }

                withContext(Dispatchers.Main) { onConnected() }
                println("[CLIENT]: Entrando a loop de listen.")
                listenLoop()
            } catch (e: IOException){
                withContext(Dispatchers.Main){
                    onDisconnected("No se pudo conectar al servidor: ${e.message}")
                }
            } catch (e: SerializationException){
                withContext(Dispatchers.Main) {
                    onDisconnected("Respuesta inválida del servidor: ${e.message}")
                }
            }
        }
    }
    
    private suspend fun listenLoop(){
        try {
            while (isConnected) {
                val line = reader?.readLine() ?: break
                println("[CLIENT]: Se recibió $line")
                withContext(Dispatchers.Main) { onMessageReceived(line) }
            }
            withContext(Dispatchers.Main) {
                onDisconnected("El servidor cerró la conexión.")
            }
        } catch (e: SocketException){
            withContext(Dispatchers.Main){
                onDisconnected("Conexión perdida: ${e.message}")
            }
        } catch (e: IOException) {
            withContext(Dispatchers.Main){
                onDisconnected("Error de lectura: ${e.message}")
            }
        }
    }

    fun send(message: String){
        scope.launch {
            try {
                writer?.println(message)
                if (writer?.checkError() == true){
                    withContext(Dispatchers.Main){
                        onDisconnected("Error al enviar mensaje.")
                    }
                }
                println("[CLIENT]: Se envió $message")
            } catch (e: IOException){
                withContext(Dispatchers.Main){
                    onDisconnected("Error al enviar mensaje: ${e.message}")
                }
            }
        }
    }
    
    fun close() {
        scope.launch {
            try {
                reader?.close()
                writer?.close()
                socket?.close()
            } catch (e: IOException) {
            } finally {
                withContext(NonCancellable) { scope.cancel() }
            }
        }
    }
}
