package mx.unam.ciencias.myp

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application


@Composable
fun Chat(
    ip: String,
    port: Int,
    username: String,
    messages: List<String>,
    onSend: (String) -> Unit,
    onLeave: () -> Unit
) {
    var msgToSend by remember { mutableStateOf("") }
    
    MaterialTheme {
        Scaffold(
            topBar = {
                TopAppBar(
                    title = { Text("Chat — $username@$ip:$port") },
                    actions = {
                        TextButton(onClick = onLeave) { Text("Salir") }
                    }
                )
            }
        ){ paddingValues ->
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues)
                    .padding(8.dp)
            ){
                LazyColumn(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                ){
                    items(messages){ msg ->
                        Text(msg, modifier = Modifier.padding(vertical = 2.dp))
                    }
                }
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(top = 8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    TextField(
                        value = msgToSend,
                        onValueChange = { msgToSend = it },
                        modifier = Modifier.weight(1f),
                        label = { Text("Mensaje") },
                        singleLine = true
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Button(onClick = {
                               if (msgToSend.isNotBlank()) {
                                   onSend(msgToSend)
                                   msgToSend = ""
                               }
                           }) {
                        Text("Enviar")
                    }
                }
            }
        }
    }
}
