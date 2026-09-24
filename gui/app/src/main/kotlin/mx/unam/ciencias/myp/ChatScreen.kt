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
import androidx.compose.ui.graphics.Color
import androidx.compose.foundation.clickable
import androidx.compose.foundation.background
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack

@Composable
fun Chat(
    ip: String,
    port: Int,
    username: String,
    messages: List<String>,
    users: Map<String,String>,
    onSend: (text: String, toUser: String?) -> Unit,
    onLeave: () -> Unit
) {
    var msgToSend by remember { mutableStateOf("") }
    var selectedUser by remember { mutableStateOf<String?>(null) }

    MaterialTheme {
        Scaffold(
            topBar = {
                // https://developer.android.com/develop/ui/compose/components/app-bars
                TopAppBar(
                    title = { Text("Chat — $username@$ip:$port") },
                    navigationIcon = {
                        IconButton(onClick = onLeave) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                                contentDescription = "Salir"
                            )
                        }
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
                Row(
                    modifier = Modifier
                        .weight(2f)
                        .fillMaxHeight()
                ) {
                    LazyColumn(
                        modifier = Modifier
                            .weight(2f)
                            .fillMaxWidth()
                    ){
                        items(messages){ msg ->
                                  Text(msg, modifier = Modifier.padding(vertical = 2.dp))
                        }
                    }
                    LazyColumn(
                        modifier = Modifier
                            .weight(0.5f)
                            .fillMaxWidth()
                    ){
                        items(users.entries.toList()){ (usr,status) ->
                            val isSelected = usr == selectedUser
                            when(status) {
                                "AWAY" -> Text(usr,
                                               modifier = Modifier
                                                   .clickable {
                                                       selectedUser = if(isSelected) null else usr
                                                   }
                                                   .background(
                                                       if(isSelected)
                                                           MaterialTheme.colors.primary.copy(alpha = 0.2f)
                                                       else
                                                           Color.Transparent
                                                   )
                                                   .padding(vertical = 2.dp),
                                               color = Color.Yellow
                                          )
                                "BUSY" -> Text(usr,
                                               modifier = Modifier
                                                   .clickable {
                                                       selectedUser = if(isSelected) null else usr
                                                   }
                                                   .background(
                                                       if(isSelected)
                                                           MaterialTheme.colors.primary.copy(alpha = 0.2f)
                                                       else
                                                           Color.Transparent
                                                   )
                                                   .padding(vertical = 2.dp),
                                               color = Color.Red
                                          )
                                "ACTIVE" -> Text(usr,
                                               modifier = Modifier
                                                   .clickable {
                                                       selectedUser = if(isSelected) null else usr
                                                   }
                                                   .background(
                                                       if(isSelected)
                                                           MaterialTheme.colors.primary.copy(alpha = 0.2f)
                                                       else
                                                           Color.Transparent
                                                   )
                                                   .padding(vertical = 2.dp),
                                               color = Color.Green
                                          )
                            }
                        }
                    }
                }

                if(selectedUser != null){
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("Privado para $selectedUser", modifier = Modifier.weight(1f))
                        TextButton(onClick = { selectedUser = null }) { Text("Cancelar") }
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
                        label = { Text(if(selectedUser!=null) "Mensaje privado para $selectedUser" else "Mensaje público") },
                        singleLine = true
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Button(onClick = {
                               if (msgToSend.isNotBlank()) {
                                   onSend(msgToSend,selectedUser)
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
