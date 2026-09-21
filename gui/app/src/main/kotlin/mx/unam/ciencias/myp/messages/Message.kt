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
    val users: String? = null
)
