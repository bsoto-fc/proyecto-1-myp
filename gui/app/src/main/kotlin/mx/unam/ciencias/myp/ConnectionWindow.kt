package mx.unam.ciencias.myp

import androidx.compose.material.*
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application

import mx.unam.ciencias.myp.utils.*

@Composable
fun ConnectionWindow(onConnect: (String,Int) -> Unit){

    var ip by remember { mutableStateOf("") }
    var port by remember { mutableStateOf("") }

    var errorMessage by remember { mutableStateOf("") }
    
    MaterialTheme {
        Scaffold(
            topBar = {
                TopAppBar(title = { Text("Conexión al servidor.") })
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
                            ip = it
                            errorMessage = ""
                        },
                        modifier = Modifier.weight(1f),
                        label = { Text("Ingresa IP del servidor.") },
                        singleLine = true,
                        isError = errorMessage.isNotEmpty()
                    )
                    
                    Spacer(modifier = Modifier.width(8.dp))
                    
                    TextField(
                        value = port,
                        onValueChange = {
                            port = it
                            errorMessage = ""
                        },
                        modifier = Modifier.weight(1f),
                        label = { Text("Ingresa puerto del servidor.") },
                        singleLine = true,
                        isError = errorMessage.isNotEmpty()
                    )
                    
                    Spacer(modifier = Modifier.width(8.dp))
                    
                    Button(onClick = {
                               if(ip.isNotBlank() && port.isNotBlank()) {
                                   try{
                                       if(!validateIPv4(ip)) 
                                           errorMessage = "Ingresa una IP válida."
                                       else if(!validatePort(port.toInt()))
                                           errorMessage = "Ingresa un puerto dentro del rango 1-65535"
                                       else {
                                           errorMessage = ""
                                           onConnect(ip,port.toInt())
                                       }
                                   } catch(e: NumberFormatException){
                                       errorMessage = "Ingresa un puerto dentro del rango 1-65535"
                                   }
                               } else
                                     errorMessage = "Ingresa ambos campos."
                           }) {
                        Text("Conectar")
                    }
                }
                if(errorMessage.isNotEmpty()){
                    Text(
                        text = errorMessage,
                        color = MaterialTheme.colors.error
                    )
                }
            }
        }
    }
}
