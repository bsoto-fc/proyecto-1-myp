package mx.unam.ciencias.myp

import androidx.compose.material.*
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application

import java.net.ConnectException
import java.net.SocketTimeoutException
import java.io.IOException

import mx.unam.ciencias.myp.sockets.*

@Composable
fun WaitingWindow(ip: String, port: Int, connectFailure: () -> Unit) {

    var serverMessage by remember { mutableStateOf("") }
    var exceptionMessage by remember { mutableStateOf("") }

    var startedConnection by remember { mutableStateOf(false) }

    if(!startedConnection){
        try {
            SocketClient.start(ip,port)
            serverMessage = SocketClient.lastReceivedMessage
        } catch (e: ConnectException) {
            exceptionMessage = "No se pudo conectar a $ip:$port. ¿El servidor está activo?"
        } catch (e: SocketTimeoutException) {
            exceptionMessage = "Timeout: El servidor no respondió en 5 segundos"
        } catch (e: IOException) {
            exceptionMessage = "Error de conexión: ${e.message}"
        }
    }
    
    MaterialTheme {
        Scaffold(
            topBar = {
                TopAppBar(title = { Text("Esperando conexión al servidor...") })
            }
        ) { paddingValues ->
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues)
            ) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    if(exceptionMessage.isNotEmpty()) {
                        Text(
                            text = exceptionMessage,
                            color = MaterialTheme.colors.error
                        )
                        
                        Spacer(modifier = Modifier.width(8.dp))

                        Button(onClick = {
                                   connectFailure()
                               }) {
                            Text("Regresar")
                        }
                    }
                    else
                        Text(
                            text = serverMessage,
                        )
                }
            }
        }
    }
}
