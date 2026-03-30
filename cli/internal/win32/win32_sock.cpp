#include "win32_platform.h"
#include "../../nc_sock.h"
#include "../../nc_system.h"

INTERNAL SOCKET
SocketFromHandle(Handle SocketHandle)
{
    SOCKET Result = (SOCKET) SocketHandle.V[0];

    return Result;
}

INTERNAL Handle
HandleFromSocket(SOCKET Socket)
{
    Handle Result = {};

    if (Socket != INVALID_SOCKET)
        Result.V[0] = (u64) Socket;

    return Result;
}

INTERNAL void
SockAddrFromSocketAddress(
    SocketAddress Address, 
    OUT SOCKADDR_STORAGE* SockAddr, 
    OUT i64* SockAddrLength
) {
    MemSet(SockAddr, 0, sizeof(*SockAddr));

    if (Address.IsIPv6) {
        sockaddr_in6* SockAddrv6 = (sockaddr_in6*) SockAddr;

        SockAddrv6->sin6_family = AF_INET6;
        SockAddrv6->sin6_port = htons(Address.Port);
        MemCpy(&SockAddrv6->sin6_addr, Address.IP, 16);
        *SockAddrLength = sizeof(sockaddr_in6);
    } else {
        SOCKADDR_IN* SockAddrv4 = (SOCKADDR_IN*) SockAddr;

        SockAddrv4->sin_family = AF_INET;
        SockAddrv4->sin_port = htons(Address.Port);
        MemCpy(&SockAddrv4->sin_addr, Address.IP, 4);
        *SockAddrLength = sizeof(SOCKADDR_IN);
    }
}

INTERNAL SocketAddress
SocketAddressFromSockAddr(SOCKADDR_STORAGE* SockAddr)
{
    SocketAddress Result = {};

    if (SockAddr->ss_family == AF_INET6) {
        sockaddr_in6* SockAddrv6 = (sockaddr_in6*) SockAddr;

        Result.IsIPv6 = TRUE;
        Result.Port = ntohs(SockAddrv6->sin6_port);
        MemCpy(Result.IP, &SockAddrv6->sin6_addr, 16);
    } else {
        sockaddr_in* SockAddrv4 = (sockaddr_in*) SockAddr;

        Result.IsIPv6 = FALSE;
        Result.Port = ntohs(SockAddrv4->sin_port);
        MemCpy(Result.IP, &SockAddrv4->sin_addr, 4);
    }

    return Result;
}

b32 
SockInit(void)
{
    WSADATA WSAData = {};
    int Result = WSAStartup(MAKEWORD(2, 2), &WSAData);

    return !Result;
}

void 
SockShutdown(void)
{
    WSACleanup();
}

Handle 
SocketOpen(SocketKind Kind, b32 IsIPv6)
{    
    int Family = (IsIPv6) ? AF_INET6 : AF_INET;
    int Type = (Kind == SOCKET_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int Protocol = (Kind == SOCKET_UDP) ? IPPROTO_UDP : IPPROTO_TCP;
    SOCKET Socket = socket(Family, Type, Protocol);

    return HandleFromSocket(Socket);
}

void 
SocketClose(Handle Socket)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return;

    closesocket(SocketFromHandle(Socket));
}

b32 
SocketSetFlag(Handle Socket, SocketFlag Flags)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return FALSE;

    SOCKET Sock = SocketFromHandle(Socket);
    b32 Result = TRUE;
    int FlagValue = 1;

    if (Flags & SOCKET_FLAG_REUSE_ADDRESS) {
        if (
            setsockopt(
                Sock, 
                SOL_SOCKET, 
                SO_REUSEADDR, 
                (CHAR const*) &FlagValue, 
                sizeof(FlagValue)
            )
        ) {
            Result = FALSE;
        }
    }

    if (Flags & SOCKET_FLAG_BROADCAST) {
        if (
            setsockopt(
                Sock, 
                SOL_SOCKET, 
                SO_BROADCAST, 
                (CHAR const*) &FlagValue, 
                sizeof(FlagValue)
            )
        ) {
            Result = FALSE;
        }
    }

    if (Flags & SOCKET_FLAG_NONBLOCKING) {
        ULONG Mode = 1;

        if (
            ioctlsocket(
                Sock,
                FIONBIO,
                &Mode
            )
        ) {
            Result = FALSE;
        }
    }

    if (Flags & SOCKET_FLAG_NODELAY) {
        if (
            setsockopt(
                Sock, 
                IPPROTO_TCP, 
                TCP_NODELAY, 
                (CHAR const*) &FlagValue, 
                sizeof(FlagValue)
            )
        ) {
            Result = FALSE;
        }
    }

    if (Flags & SOCKET_FLAG_KEEPALIVE) {
        if (
            setsockopt(
                Sock, 
                SOL_SOCKET, 
                SO_KEEPALIVE, 
                (CHAR const*) &FlagValue, 
                sizeof(FlagValue)
            )
        ) {
            Result = FALSE;
        }
    }

    return Result;
}

b32 
SocketBind(Handle Socket, SocketAddress Address)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return FALSE;

    SOCKADDR_STORAGE SockAddr = {};
    i64 SockAddrLength = 0;

    SockAddrFromSocketAddress(Address, &SockAddr, &SockAddrLength);

    int BindResult = bind(
        SocketFromHandle(Socket), 
        (SOCKADDR*) &SockAddr, 
        SockAddrLength
    );

    return !BindResult;
}

b32 
SocketListen(Handle Socket, u32 Backlog)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return FALSE;

    int ListenResult = listen(
        SocketFromHandle(Socket),
        (int) Backlog
    );

    return !ListenResult;
}

Handle 
SocketAccept(Handle Socket, OUT SocketAddress* ClientAddress)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return EMPTY_HANDLE_VALUE;

    SOCKADDR_STORAGE SockAddr = {};
    i64 SockAddrLength = (i64) sizeof(SockAddr);
    SOCKET Client = accept(
        SocketFromHandle(Socket),
        (SOCKADDR*) &SockAddr,
        (INT*) &SockAddrLength
    );

    if (Client != INVALID_SOCKET && ClientAddress)
        *ClientAddress = SocketAddressFromSockAddr(&SockAddr);

    return HandleFromSocket(Client);
}

b32 
SocketConnect(Handle Socket, SocketAddress Address)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return FALSE;

    SOCKADDR_STORAGE SockAddr = {};
    i64 SockAddrLength = 0;

    SockAddrFromSocketAddress(Address, &SockAddr, &SockAddrLength);

    int ConnResult = connect(
        SocketFromHandle(Socket), 
        (SOCKADDR*) &SockAddr, 
        SockAddrLength
    );

    return !ConnResult;
}

void 
SocketShutdown(Handle Socket, SocketTransferKind TransferKind)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return;

    int ShutdownMethod = SD_BOTH;

    switch (TransferKind) {
        case SOCKET_XFER_KIND_READ: { 
            ShutdownMethod = SD_RECEIVE;
        } break;

        case SOCKET_XFER_KIND_WRITE: { 
            ShutdownMethod = SD_SEND; 
        } break;

        case SOCKET_XFER_KIND_BOTH: { 
            ShutdownMethod = SD_BOTH; 
        } break;
    }

    shutdown(SocketFromHandle(Socket), ShutdownMethod);
}

u64 
SocketSend(Handle Socket, void* Data, u64 DataSize)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return 0;

    SOCKET Sock = SocketFromHandle(Socket);
    u64 BytesSent = 0;

    for ( ; BytesSent < DataSize; ) {
        i64 ChunkSize = MIN((i64) (DataSize - BytesSent), I64_MAX);
        i64 Sent = send(
            Sock,
            (char*) Data + BytesSent, 
            ChunkSize, 
            0
        );

        if (Sent == SOCKET_ERROR)
            break;

        BytesSent += (u64) Sent;
    }

    return BytesSent;
}

u64 
SocketRecv(Handle Socket, void* Buffer, u64 BufferSize)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return 0;

    i64 ChunkSize = MIN((i64) BufferSize, I64_MAX);
    i64 Receieved = recv(
        SocketFromHandle(Socket),
        (char*) Buffer,
        ChunkSize,
        0
    );

    if (Receieved == SOCKET_ERROR || Receieved < 0)
        return 0;

    return (u64) Receieved;
}

u64 
SocketSendTo(Handle Socket, void* Data, u64 DataSize, SocketAddress Dst)
{
    if (Socket == EMPTY_HANDLE_VALUE)
        return 0;

    SOCKADDR_STORAGE SockAddr = {};
    i64 SockAddrLength = 0;

    SockAddrFromSocketAddress(Dst, &SockAddr, &SockAddrLength);

    i64 ChunkSize = MIN((i64) DataSize, I64_MAX);
    i64 Sent = sendto(
        SocketFromHandle(Socket),
        (char*) Data,
        ChunkSize,
        0,
        (SOCKADDR*) &SockAddr,
        SockAddrLength
    );

    if (Sent == SOCKET_ERROR)
        return 0;

    return (u64) Sent;
}

SocketRecvResult 
SocketRecvFrom(Handle Socket, void* Buffer, u64 BufferSize)
{
    SocketRecvResult Result = {};

    if (Socket == EMPTY_HANDLE_VALUE)
        return Result;

    SOCKADDR_STORAGE SockAddr = {};
    i64 SockAddrLength = sizeof(SockAddr);
    i64 ChunkSize = MIN((i64) BufferSize, I64_MAX);
    i64 Received = recvfrom(
        SocketFromHandle(Socket),
        (char*) Buffer,
        ChunkSize,
        0,
        (SOCKADDR*) &SockAddr,
        (INT*) &SockAddrLength
    );

    if (Received > 0) {
        Result.BytesRead = (u64) Received;
        Result.From = SocketAddressFromSockAddr(&SockAddr);
    }

    return Result;
}

SocketAddress 
SockAddrIPv4(u8 A, u8 B, u8 C, u8 D, u16 Port)
{
    SocketAddress Result = {
        {
            A,
            B,
            C,
            D
        },
        Port,
        FALSE
    };

    return Result;
}

SocketAddress 
SockAddrAny(u16 Port)
{
    SocketAddress Result = {};

    Result.Port = Port;

    return Result;
}

SocketAddress 
SockAddrLocal(u16 Port)
{
    return SockAddrIPv4(127, 0, 0, 1, Port);
}

b32 
SocketResolveAddr(
    Arena* MemPool, 
    Str8 Host, 
    u16 Port, 
    OUT SocketAddress* Address
) {
    b32 Result = FALSE;
    TempArena Scratch = GetScratch(&MemPool, 1);
    Str8 HostCopy = ArenaPushStrCpy(Scratch.MemPool, Host);
    ADDRINFOA Hints = {};
    ADDRINFOA* AddrInfo = NULL;

    Hints.ai_family = AF_UNSPEC;
    Hints.ai_socktype = SOCK_STREAM;

    int AddrInfoResult = getaddrinfo(
        (char*) HostCopy.Str,
        NULL,
        &Hints,
        &AddrInfo
    );

    if (!AddrInfoResult && AddrInfo) {
        SOCKADDR_STORAGE Storage = {};

        MemCpy(&Storage, AddrInfo->ai_addr, AddrInfo->ai_addrlen);
        *Address = SocketAddressFromSockAddr(&Storage);
        Address->Port = Port;
        Result = TRUE;
        freeaddrinfo(AddrInfo);
    }

    ReleaseScratch(Scratch);

    return Result;
}
