package mx.unam.ciencias.myp.messages

sealed class RoomCreationStatus {
    object Idle : RoomCreationStatus()
    data class Pending(val roomName: String) : RoomCreationStatus()
    data class Error(val message: String) : RoomCreationStatus()
}
