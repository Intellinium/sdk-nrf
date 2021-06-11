@file:Repository("https://maven.pkg.jetbrains.space/public/p/space/maven")
@file:Repository("https://jcenter.bintray.com")

@file:DependsOn("org.jetbrains:space-sdk-jvm:72091-beta")
@file:DependsOn("io.github.microutils:kotlin-logging-jvm:2.0.8")
@file:DependsOn("io.ktor:ktor-client-core:1.6.0")
@file:DependsOn("io.ktor:ktor-client-cio:1.6.0")
@file:DependsOn("io.ktor:ktor-client-apache:1.6.0")

import space.jetbrains.api.runtime.SpaceHttpClient
import io.ktor.client.*
import kotlinx.coroutines.runBlocking
import space.jetbrains.api.runtime.resources.chats
import space.jetbrains.api.runtime.withServiceAccountTokenSource

val space = SpaceHttpClient(HttpClient()).withServiceAccountTokenSource(
    clientId = "2af60f97-08a8-4ddc-81e1-9f5057de768a",
    clientSecret = "3e68c55bc3684e4dd847c78488b0f6e81ac0d880da8e74a8cf2b30d1e6ac0506",
    serverUrl = "https://intellinium.jetbrains.space"
)

runBlocking {
    space.chats.channels.messages.sendTextMessage(
        channelId = "3RHRvU3bsWEy",
        text = "A new nrf manifest version available.\nPlease run ```git pull``` and ```west update``` in your ```ncs-itl/nrf``` folder"
    )
}
