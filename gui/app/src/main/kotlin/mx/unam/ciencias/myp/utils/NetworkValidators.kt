package mx.unam.ciencias.myp.utils

import kotlin.text.Regex

fun validateIPv4(ip: String) : Boolean {
    val pattern = Regex("^((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.?\\b){4}$")
    return pattern.matches(ip)
}

fun validatePort(port: Int): Boolean {
    return port in 1..65535
}
