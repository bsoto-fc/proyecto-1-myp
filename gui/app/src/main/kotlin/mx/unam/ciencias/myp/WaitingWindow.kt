package mx.unam.ciencias.myp

import androidx.compose.material.*
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application

import mx.unam.ciencias.myp.sockets.*

@Composable
fun WaitingWindow(ip: String, port: String) {
    MaterialTheme {
        Scaffold(
            topBar = {
                TopAppBar(title = { Text("Esperando conexión al servidor.") })
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
                    TextField(
                        value = ip,
                        onValueChange = {
                        },
                        modifier = Modifier.weight(1f),
                        label = { Text("Ingresa IP del servidor.") },
                        singleLine = true,
                    )
                    
                    Spacer(modifier = Modifier.width(8.dp))
                    
                    TextField(
                        value = port,
                        onValueChange = {
                        },
                        modifier = Modifier.weight(1f),
                        label = { Text("Ingresa puerto del servidor.") },
                        singleLine = true,
                    )
                    
                    Spacer(modifier = Modifier.width(8.dp))
                    
                    Button(onClick = {
                               socket(ip,port.toInt())
                           }) {
                        Text("Conectar")
                    }
                }
            }
        }
    }
}
