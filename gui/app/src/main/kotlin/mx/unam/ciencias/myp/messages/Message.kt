package mx.unam.ciencias.myp.messages

import kotlinx.serialization.*
import kotlinx.serialization.json.*

val json = Json {
    explicitNulls = false
    ignoreUnknownKeys = true
}

@Serializable
data class Message(
    val type: String,
    val username: String? = null,
    val operation: String? = null,
    val result: String? = null,
    val extra: String? = null,
    val status: String? = null,
    val text: String? = null,
    val roomname: String? = null,
    val usernames: List<String>? = null,
    val users: Map<String,String>? = null
)

data class Invitation(val roomname: String, val fromUsername: String)

fun determineActionFromJSON(
    messageJSON: Message?,
    users: MutableMap<String,String>? = null,
    messages: MutableList<String>? = null,
    privateConvos: MutableMap<String,MutableList<String>>? = null,
    roomConvos: MutableMap<String,MutableList<String>>? = null,
    onRoomStatusChange: ((RoomCreationStatus) -> Unit)? = null,
    roomUsers: MutableMap<String,String>? = null,
    pendingInvitations: MutableList<Invitation>? = null
){
    when(messageJSON?.type) {
        "NEW_USER" -> {
            val status = messageJSON.status ?: "ACTIVE"
            val username = messageJSON.username ?: return
            users?.put(username,status)
        }
        "NEW_STATUS" -> {
            val username = messageJSON.username ?: return
            val status = messageJSON.status ?: return
            users?.put(username, status)
        }
        "USER_LIST" -> {
            val usersJSON = messageJSON.users
            users?.clear()
            usersJSON?.forEach { (username,status) ->
                users?.put(username,status)
            }
        }
        "TEXT_FROM" -> {
            val fromUser = messageJSON.username ?: return
            val text = messageJSON.text ?: return
            val updatedMessages = privateConvos?.get(fromUser)?.toMutableList() ?: mutableListOf()
            updatedMessages.add("$fromUser: $text")
            // Reemplazar lista para que compose pueda actualizar los mensajes.
            privateConvos?.put(fromUser,updatedMessages)
        }
        "PUBLIC_TEXT_FROM" -> {
            val username = messageJSON.username ?: "INVALID"
            val text = messageJSON.text ?: "INVALID"
            messages?.add("$username: $text")
        }
        "JOINED_ROOM" -> {
            val roomname = messageJSON.roomname ?: return
            val username = messageJSON.username ?: return
            val updatedMessages = roomConvos?.get(roomname)?.toMutableList() ?: mutableListOf()
            updatedMessages.add("$username se unió a la sala.")
            roomConvos?.put(roomname, updatedMessages)
        }
        "ROOM_USER_LIST" -> {
            val roomname = messageJSON.roomname?: return
            val users = messageJSON.users?: return
            roomUsers?.clear()
            users.forEach { (username,status) ->
                roomUsers?.put(username,status)
            }

        }
        "ROOM_TEXT_FROM" -> {
            val roomname = messageJSON.roomname ?: return
            val username = messageJSON.username ?: "INVALID"
            val text = messageJSON.text ?: "INVALID"
            val updatedMessages = roomConvos?.get(roomname)?.toMutableList() ?: mutableListOf()
            updatedMessages.add("$username: $text")
            roomConvos?.put(roomname, updatedMessages)
        }
        "LEFT_ROOM" -> {
            val roomname = messageJSON.roomname ?: return
            val username = messageJSON.username ?: return
            val updatedMessages = roomConvos?.get(roomname)?.toMutableList() ?: mutableListOf()
            updatedMessages.add("$username abandonó la sala.")
            roomConvos?.put(roomname, updatedMessages)
        }
        "INVITATION" -> {
            val roomname = messageJSON.roomname ?: return
            val fromUsername = messageJSON.username ?: return
            pendingInvitations?.add(Invitation(roomname, fromUsername))
        }
        "DISCONNECTED" -> {
            val username = messageJSON.username ?: return
            users?.remove(username)
            privateConvos?.remove(username)
        }
        "RESPONSE" -> {
            when(messageJSON?.operation) {
                "NEW_ROOM" -> {
                    when(messageJSON?.result) {
                        "SUCCESS" -> {
                            val roomName = messageJSON?.extra ?: return
                            roomConvos?.putIfAbsent(roomName, mutableListOf())
                            onRoomStatusChange?.invoke(RoomCreationStatus.Idle)
                        }
                        "ROOM_ALREADY_EXISTS" -> {
                            onRoomStatusChange?.invoke(RoomCreationStatus.Error("Sala ya existente."))
                        }
                    }
                }
                "JOIN_ROOM" -> {
                    val roomname = messageJSON?.extra ?: return
                    when(messageJSON?.result) {
                        "SUCCESS" -> {
                            roomConvos?.putIfAbsent(roomname, mutableListOf())
                            pendingInvitations?.removeAll { it.roomname == roomname }
                        }
                        "NO_SUCH_ROOM", "NOT_INVITED" -> {
                            pendingInvitations?.removeAll { it.roomname == roomname }
                        }
                    }
                }
                "TEXT" -> {
                    when(messageJSON?.result) {
                        "NO_SUCH_USER" -> {
                            
                        }
                    }
                }
            }
        }
    }
}


fun getFetchUserListRequest(): String {
    val userListRequest = Message(type = "USERS")
    return json.encodeToString(userListRequest)
}

fun getDisconnectRequest(): String {
    val disconnectRequest = Message(type = "DISCONNECT")
    return json.encodeToString(disconnectRequest)
}

fun getFetchRoomUserListRequest(roomname: String): String {
    val userListRequest = Message(type = "ROOM_USERS", roomname = roomname)
    return json.encodeToString(userListRequest)
}

fun getJoinRoomRequest(roomname: String): String {
    val joinRoomRequest = Message(type = "JOIN_ROOM", roomname = roomname)
    return json.encodeToString(joinRoomRequest)
}
 
fun getLeaveRoomRequest(roomname: String): String {
    val leaveRoomRequest = Message(type = "LEAVE_ROOM", roomname = roomname)
    return json.encodeToString(leaveRoomRequest)
}
