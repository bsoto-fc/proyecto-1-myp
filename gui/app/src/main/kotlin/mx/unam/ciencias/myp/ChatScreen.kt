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
import androidx.compose.foundation.border
import androidx.compose.ui.window.Dialog
import androidx.compose.foundation.shape.RoundedCornerShape

import mx.unam.ciencias.myp.messages.RoomCreationStatus
import mx.unam.ciencias.myp.messages.Invitation

@Composable
fun Chat(
    ip: String,
    port: Int,
    username: String,
    messages: List<String>,
    users: Map<String,String>,
    privateConvos: Map<String,MutableList<String>>,
    roomConvos: Map<String,MutableList<String>>,
    onSend: (text: String, toUser: String?, toRoom:String?) -> Unit,
    roomCreationStatus: RoomCreationStatus,
    onLeave: () -> Unit ,
    onNewRoom: (roomname: String) -> Unit,
    onRoomInvite: (roomname:String?, usersToInvite: List<String>) -> Unit,
    onRoomUsersRequest: (String) -> Unit,
    roomUsers: Map<String,String>,
    pendingInvitations: List<Invitation>,
    onAcceptInvitation: (roomname: String) -> Unit,
    onDeclineInvitation: (invitation: Invitation) -> Unit,
    onLeaveRoom: (roomname: String) -> Unit
){
    var msgToSend by remember { mutableStateOf("") }
    var selectedUser by remember { mutableStateOf<String?>(null) }
    var selectedRoom by remember { mutableStateOf<String?>(null) }

    var inviting by remember { mutableStateOf(false) }

    val displayedMessages = when {
        selectedRoom != null -> roomConvos.get(selectedRoom) ?: emptyList()
        selectedUser != null -> privateConvos.get(selectedUser) ?: emptyList()
        else -> messages
    }

    val displayedUsers = when {
        inviting -> users
        selectedRoom != null -> roomUsers
        else ->users
    }

    var showNewRoomDialog by remember { mutableStateOf(false) }

    var usersToInvite = remember { mutableStateListOf<String>() }

    LaunchedEffect(roomCreationStatus) {
        if (roomCreationStatus == RoomCreationStatus.Idle) {
            showNewRoomDialog = false
        }
    }

    val room = selectedRoom

    LaunchedEffect(room) {
        if (room != null) {
            onRoomUsersRequest(room)
        }
    }

    LaunchedEffect(roomConvos.keys) {
        if (selectedRoom != null && selectedRoom !in roomConvos.keys) {
            selectedRoom = null
        }
    }

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
            if(showNewRoomDialog){
                NewRoomDialog(
                    isLoading = roomCreationStatus is RoomCreationStatus.Pending,
                    errorMessage = (roomCreationStatus as? RoomCreationStatus.Error)?.message,
                    onDismissRequest = {
                        showNewRoomDialog = false
                    },
                    onConfirmation = { roomName ->
                        if(roomName.isNotBlank()) {
                            onNewRoom(roomName)
                        }
                    }
                )
            }
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues)
                    .padding(8.dp)
            ){
                if(pendingInvitations.isNotEmpty()){
                    Column(modifier = Modifier.fillMaxWidth().padding(bottom = 8.dp)){
                        Text("Invitaciones", style = MaterialTheme.typography.h6)
                        pendingInvitations.forEach { invitation ->
                            Row(
                                modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
                                verticalAlignment = Alignment.CenterVertically
                            ){
                                Text(
                                    "${invitation.fromUsername} te invitó a ${invitation.roomname}",
                                    modifier = Modifier.weight(1f)
                                )
                                TextButton(onClick = { onDeclineInvitation(invitation) }) { Text("Rechazar")}
                                Spacer(modifier = Modifier.width(8.dp))
                                Button(onClick = { onAcceptInvitation(invitation.roomname)}) { Text("Aceptar") }
                            }
                        }
                    }
                }
                Row(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                        .fillMaxHeight()
                        .padding(paddingValues)
                        .padding(8.dp)
                ) {
                    Column(
                        modifier = Modifier
                            .weight(0.8f)
                            .fillMaxSize()
                            .padding(paddingValues)
                            .padding(8.dp)
                    ){
                        LazyColumn(
                            modifier = Modifier
                                .weight(1f)
                                .fillMaxWidth()
                                .fillMaxHeight()
                                .padding(8.dp)
                                .border(
                                    width = 1.dp,
                                    color = Color.Gray
                                )
                        ){
                            item {
                                Text(
                                    text = "Subchats",
                                    style = MaterialTheme.typography.h5
                                )
                                Text(
                                    text = "Público",
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .clickable {
                                            selectedUser = null
                                            selectedRoom = null
                                        }
                                        .padding(vertical = 6.dp)
                                )
                            }
                            items(privateConvos.keys.toList()) { user ->
                                      Text(
                                          text = "Privado: $user",
                                          modifier = Modifier
                                              .fillMaxWidth()
                                              .clickable {
                                                  selectedUser = user
                                                  selectedRoom = null
                                              }
                                              .background(
                                                  if (selectedUser == user) {
                                                      MaterialTheme.colors.primary.copy(alpha = 0.2f)
                                                  } else {
                                                      Color.Transparent
                                                  }
                                              )
                                              .padding(vertical = 6.dp)
                                      )
                            }
                            items(roomConvos.keys.toList()) { room ->
                                      Text(
                                          text = "Sala: $room",
                                          modifier = Modifier
                                              .fillMaxWidth()
                                              .clickable {
                                                  selectedRoom = room
                                                  selectedUser = null
                                              }
                                              .background(
                                                  if (selectedRoom == room) {
                                                      MaterialTheme.colors.primary.copy(alpha = 0.2f)
                                                  } else {
                                                      Color.Transparent
                                                  }
                                              )
                                              .padding(vertical = 6.dp)
                                      )
                            }
                        }
                        Button(
                            onClick = {
                                showNewRoomDialog = true;
                            },
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Text(
                                text = "Crear sala",
                                modifier = Modifier.padding(vertical = 6.dp)
                            )
                        }
                    }
                    LazyColumn(
                        modifier = Modifier
                            .weight(2f)
                            .fillMaxWidth()
                            .fillMaxHeight()
                            .padding(paddingValues)
                            .padding(8.dp)
                            .border(
                                width = 1.dp,
                                color = Color.Gray
                            )
                    ){
                        item {
                            Text(
                                text = "Mensajes",
                                style = MaterialTheme.typography.h5,
                                modifier = Modifier.padding(bottom = 8.dp)
                            )
                        }
                        items(displayedMessages){ msg ->
                                  Text(msg, modifier = Modifier.padding(vertical = 2.dp))
                        }
                    }
                    LazyColumn(
                        modifier = Modifier
                            .weight(0.5f)
                            .fillMaxWidth()
                            .fillMaxHeight()
                            .padding(paddingValues)
                            .padding(8.dp)
                            .border(
                                width = 1.dp,
                                color = Color.Gray
                            )
                    ){
                        item {
                            Text(
                                text = "Usuarios",
                                style = MaterialTheme.typography.h5,
                                modifier = Modifier.padding(bottom = 8.dp)
                            )
                        }
                        items(displayedUsers.entries.toList()){ (usr,status) ->
                            val isSelected = usr == selectedUser
                            when(status) {
                                "AWAY" -> Text(usr,
                                               modifier = Modifier
                                                   .clickable {
                                                       if(inviting) {
                                                           if(usr in usersToInvite) {
                                                               usersToInvite.remove(usr)
                                                           } else {
                                                               usersToInvite.add(usr)
                                                           }
                                                       } else
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
                                                       if(inviting) {
                                                           if(usr in usersToInvite) {
                                                               usersToInvite.remove(usr)
                                                           } else {
                                                               usersToInvite.add(usr)
                                                           }
                                                       } else
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
                                                       if(inviting) {
                                                           if(usr in usersToInvite) {
                                                               usersToInvite.remove(usr)
                                                           } else {
                                                               usersToInvite.add(usr)
                                                           }
                                                       } else
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

                if(inviting){
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("Invitar a ${usersToInvite.joinToString(", ")} a sala.", modifier = Modifier.weight(1f))
                        TextButton(onClick = {
                                       inviting = false
                                   }) { Text("Cancelar") }
                        Spacer(modifier = Modifier.width(8.dp))
                        Button(onClick = {
                                   onRoomInvite(selectedRoom,usersToInvite)
                                   inviting = false
                               }) {
                            Text("Enviar")
                        }
                    }
                }
                if(selectedUser != null && !inviting){
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
                    if(!inviting) {
                        TextField(
                            value = msgToSend,
                            onValueChange = { msgToSend = it },
                            modifier = Modifier.weight(1f),
                            label = { Text(
                                          when {
                                              selectedRoom != null -> "Mensaje para la sala $selectedRoom"  
                                                                      selectedUser != null -> "Mensaje privado para $selectedUser "   
                                              else -> "Mensaje público"
                                          }
                                      )
                            },
                            singleLine = true
                        )
                        if(selectedRoom != null ){
                            Spacer(modifier = Modifier.width(8.dp))
                            Button(onClick = {
                                       inviting = true
                                   }) {
                                Text("Invitar")
                            }
                            Spacer(modifier = Modifier.width(8.dp))
                            Button(onClick = {
                                       val roomToLeave = selectedRoom
                                       if (roomToLeave != null) {
                                           onLeaveRoom(roomToLeave)
                                           selectedRoom = null
                                       }
                                   }) {
                                Text("Salir de sala")
                            }
                        }
                        Spacer(modifier = Modifier.width(8.dp))
                        Button(onClick = {
                                   onSend(msgToSend,selectedUser,selectedRoom)
                                   msgToSend = ""
                               }) {
                            Text("Enviar")
                        }
                    }
                }
            }
        }
    }
}

// https://developer.android.com/develop/ui/compose/components/dialog
@Composable
fun NewRoomDialog(
    isLoading: Boolean,
    errorMessage: String?,
    onDismissRequest: () -> Unit,
    onConfirmation: (String) -> Unit,
) {
    
    var roomname by remember { mutableStateOf("") }
    
    Dialog(onDismissRequest = { onDismissRequest() }) {
        // Draw a rectangle shape with rounded corners inside the dialog
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .height(375.dp)
                .padding(16.dp),
            shape = RoundedCornerShape(16.dp),
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize(),
                verticalArrangement = Arrangement.Center,
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                Text(
                    text = "Nueva habitación",
                    modifier = Modifier.padding(16.dp),
                )
                Row(
                    modifier = Modifier
                        .fillMaxWidth(),
                    horizontalArrangement = Arrangement.Center,
                ) {
                    TextField(
                        value = roomname,
                        onValueChange = { roomname = it },
                        modifier = Modifier.weight(1f),
                        label = { Text("Nombre de nueva sala.")
                        },
                        singleLine = true
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    if (isLoading) {
                        CircularProgressIndicator(modifier = Modifier.size(20.dp))
                    }
                    errorMessage?.let {
                        Text(it, color = MaterialTheme.colors.error)
                    }
                }
                Row(
                    modifier = Modifier
                        .fillMaxWidth(),
                    horizontalArrangement = Arrangement.Center,
                ) {
                    TextButton(
                        onClick = { onDismissRequest() },
                        modifier = Modifier.padding(8.dp),
                    ) {
                        Text("Salir")
                    }
                    TextButton(
                        onClick = { onConfirmation(roomname) },
                        modifier = Modifier.padding(8.dp),
                    ) {
                        Text("Confirmar")
                    }
                }
            }
        }
    }
}
