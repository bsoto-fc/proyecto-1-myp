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

fun determineActionFromJSON(
    messageJSON: Message?,
    users: MutableMap<String,String>? = null,
    messages: MutableList<String>? = null
){
    when(messageJSON?.type) {
        "NEW_USER" -> {
            val status = messageJSON?.status ?: "ACTIVE"
            val username = messageJSON?.username ?: "INVALID"
            users?.put(username,status)
        }
        "PUBLIC_TEXT_FROM" -> {
            val username = messageJSON?.username ?: "INVALID"
            val text = messageJSON?.text ?: "INVALID"
            messages?.add("$username: $text")
        }
        "USER_LIST" -> {
            val usersJSON = messageJSON?.users
            users?.clear()
            usersJSON?.forEach { (username,status) ->
                users?.put(username,status)
            }
        }
        "DISCONNECTED" -> {
            val username = messageJSON?.username
            users?.remove(username)
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
