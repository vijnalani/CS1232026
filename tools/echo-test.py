#!/usr/bin/env python3
"""Перевірка коректності echo-сервера (ЛР 4).

Використання:  python3 echo-test.py ПОРТ [ХОСТ]
Код повернення: 0 — усі тести пройдено, 1 — є провали, 2 — сервер недоступний.
Лише стандартна бібліотека Python 3.8+.
"""
import asyncio, hashlib, os, random, socket, sys, time

HOST, PORT = "127.0.0.1", 0
TIMEOUT = 20


async def connect():
    return await asyncio.wait_for(asyncio.open_connection(HOST, PORT), TIMEOUT)


async def roundtrip(reader, writer, data):
    writer.write(data)
    await writer.drain()
    got = await asyncio.wait_for(reader.readexactly(len(data)), TIMEOUT)
    return got == data


async def t_single():
    """Одне коротке повідомлення повертається байт у байт."""
    r, w = await connect()
    ok = await roundtrip(r, w, "привіт, ядро\n".encode())
    w.close()
    return ok


async def t_sequence():
    """Сто повідомлень різного розміру в одному з'єднанні."""
    r, w = await connect()
    rnd = random.Random(1)
    for _ in range(100):
        if not await roundtrip(r, w, os.urandom(rnd.randint(1, 10000))):
            return False
    w.close()
    return True


async def t_concurrent(n=50):
    """50 клієнтів одночасно, кожен — 50 обмінів. Перевіряє, що клієнти не заважають одне одному."""
    async def one(i):
        r, w = await connect()
        rnd = random.Random(i)
        for _ in range(50):
            if not await roundtrip(r, w, os.urandom(rnd.randint(1, 2000))):
                return False
        w.close()
        return True
    return all(await asyncio.gather(*(one(i) for i in range(n))))


async def t_big(size=8 * 1024 * 1024):
    """8 МіБ в одному з'єднанні: ловить втрату даних при частковому write().
    Клієнт пише й читає одночасно, тож буфер сервера рано чи пізно заповнюється."""
    r, w = await connect()
    data = os.urandom(size)

    async def send():
        for i in range(0, size, 65536):
            w.write(data[i:i + 65536])
            await w.drain()

    sender = asyncio.ensure_future(send())
    got = await asyncio.wait_for(r.readexactly(size), 20)
    await sender
    w.close()
    return hashlib.sha256(got).digest() == hashlib.sha256(data).digest()


async def t_rude_clients(n=20):
    """Клієнти, що обривають з'єднання (RST) посеред обміну. Сервер має вижити."""
    for _ in range(n):
        s = socket.create_connection((HOST, PORT), timeout=TIMEOUT)
        s.sendall(os.urandom(100000))
        s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, b"\x01\x00\x00\x00\x00\x00\x00\x00")
        s.close()
    await asyncio.sleep(0.3)
    return await t_single()


async def t_idle_many(n=300):
    """300 відкритих мовчазних з'єднань, потім обмін на кожному."""
    conns = [await connect() for _ in range(n)]
    ok = True
    for r, w in conns:
        ok = ok and await roundtrip(r, w, b"ping")
    for _, w in conns:
        w.close()
    return ok


TESTS = [t_single, t_sequence, t_concurrent, t_big, t_rude_clients, t_idle_many]


async def main():
    failed = 0
    for t in TESTS:
        start = time.monotonic()
        try:
            ok = await t()
            err = ""
        except Exception as e:  # таймаут, обрив з'єднання тощо
            ok, err = False, f"  ({type(e).__name__}: {e})"
        dt = time.monotonic() - start
        print(f"{'PASS' if ok else 'FAIL'}  {t.__name__:<16} {dt:6.2f} с  {t.__doc__.splitlines()[0]}{err}")
        failed += not ok
    print(f"\n{len(TESTS) - failed}/{len(TESTS)} тестів пройдено")
    return 1 if failed else 0


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3):
        print(__doc__)
        sys.exit(2)
    PORT = int(sys.argv[1])
    if len(sys.argv) == 3:
        HOST = sys.argv[2]
    try:
        socket.create_connection((HOST, PORT), timeout=3).close()
    except OSError as e:
        print(f"сервер на {HOST}:{PORT} недоступний: {e}")
        sys.exit(2)
    sys.exit(asyncio.run(main()))
