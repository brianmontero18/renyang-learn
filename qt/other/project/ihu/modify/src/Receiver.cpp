/*
 *  IHU -- I Hear U, easy VoIP application using Speex and Qt
 *
 *  Copyright (C) 2003-2008 Matteo Trotta - <mrotta@users.sourceforge.net>
 *
 *  http://ihu.sourceforge.net
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 */

#include "Error.h"
#include "Config.h"
#include "Receiver.hpp"
#include "PacketHandler.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <qobject.h>
#include <qstring.h>
#include <qtimer.h>
#include <qsocketnotifier.h>

#define MAXBUFSIZE 65536
#define IN_BUFSIZE 16384

#define CHECK_TICKTIME 1000

#define IHU_ANONYMOUS "anonymous"

Receiver::Receiver(Rsa *r) : rsa(r)
{
#ifdef REN_DEBUG
	qWarning("Receiver::Receiver(Rsa *r) : rsa(r)");
#endif
	setName("Receiver");

	working = false;
	received = false;

	if ((inputBuffer = (char *)malloc(IN_BUFSIZE))==NULL)
		throw Error(Error::IHU_ERR_MEMORY);
	if ((streamBuffer = (char *)malloc(MAXBUFSIZE))==NULL)
		throw Error(Error::IHU_ERR_MEMORY);

	s = -1;
	notifier = NULL;
	calen = sizeof(ca);

	blowfish = NULL;
	outFile = NULL;

	checkTimer = new QTimer(this);

	reset();
	resetStream();

	connect(checkTimer, SIGNAL(timeout()), this, SLOT(checkConnection()));
}

Receiver::~Receiver(void)
{
#ifdef REN_DEBUG
	qWarning("Receiver::~Receiver(void)");
#endif
	if (inputBuffer)
		free(inputBuffer);
	if (streamBuffer)
		free(streamBuffer);
	if (outFile)
		fclose(outFile);
}

void Receiver::reset()
{
#ifdef REN_DEBUG
	qWarning("Receiver::reset()");
#endif
	ihu_refuse = false;
	ihu_reply = false;
	ihu_abort = false;
	connected = false;
	nodecrypt = false;
	active = false;
	total = bytes = 0;
	callerName = IHU_ANONYMOUS;
}

void Receiver::resetStream()
{
#ifdef REN_DEBUG
	qWarning("Receiver::resetStream()");
#endif
	streamLen = 0;
	streamPtr = streamBuffer;
	sync = STREAM_READ_DATA;
}

void Receiver::dump(QString file)
{
#ifdef REN_DEBUG
	qWarning(QString("Receiver::dump(QString %1)").arg(file));
#endif
	if (!file.isEmpty())
	{
		outFile = fopen(file.ascii(), "ab");
		if (outFile==NULL)
			throw Error(QString("%1: %2").arg(file).arg(strerror(errno)));
	}
	else
	{
		if (outFile)
		{
			fclose(outFile);
			outFile = NULL;
		}
	}
}

// renyang - 開始接收對方的資料
// renyang - socket代表client端的socket file descriptor
// renyang - proto可能是IHU_TCP或IHU_UDP
// renyang-TODO - 要加入IHU_SCTP的部分
void Receiver::start(int socket, int proto)
{
#ifdef REN_DEBUG
	qWarning(QString("Receiver::start(int %1, int %2)").arg(socket).arg(proto));;
#endif
	s = socket;
	protocol = proto;
	// renyang - 連線之後, 可以取得對方的sockaddr_in資料
	// renyang - 由socket取得對方的struct sockaddr
	// renyang - 此s代表client端的socket file descriptor
	::getpeername(s, (struct sockaddr *)&ca, &calen);

	// renyang - 設定sync的型態為STREAM_READ_DATA
	resetStream();
	// renyang - 設定ihu_refuse, ihu_reply, ihu_abort, connected,...為false
	reset();
	// renayng - working=ture, 表示正在響鈴
	go();

	// renyang - 新增一個QSocketNotifier, 當有資料可以讀時, emit activated()
	// renyang - 用來判斷對方是否有傳資料進來
	notifier = new QSocketNotifier(s, QSocketNotifier::Read, this);
	// renyang - activated(int)其參數是表示哪一個socket file descriptor被觸發
	// renyang - 表示client端有傳送資料過來
	// renyang - 每格一段時間接收一次資料
	connect(notifier,SIGNAL(activated(int)),this, SLOT(receive()));
	// renyang - 因為tcp所接收到的第一個資料是connection，所以，不需要
	// renyang - 但是, udp所接收到的第一個資料就是資料啦, 所以要比tcp多接收一次
	if (received)
	{
		// renyang - 若是udp的話, 則直接開始接收資料, 因udp只要是有資料送過來就直接接收@@@
		switch(protocol)
		{
			case IHU_UDP:
				// renyang - 因為是connection-less的關係, 所以先接收第一筆資料???
				receive();
				break;
		}
		// renyang - 不論是tcp或是udp均會處理目前的函式
		// renyang - 把這一個socket, protocol, ca送到Transmitter, 讓Transmitter可以傳送資料, 給peer端
		// renyang - 只有當是waitCalls才會建立一個Transmitter的一個socket用來傳送資料
		emit newSocket(s, protocol, ca);
	}
	// renyang - 開始倒數計時, false表示timeout一次, 就呼叫timeout()一次
	checkTimer->start(CHECK_TICKTIME, false);
}

// renyang - 每隔一段時間去判斷對方的狀況
void Receiver::checkConnection()
{
#ifdef REN_DEBUG
	qWarning("Receiver::checkConnection()");
#endif
	if (received)
	{
		// renyang - 當是waiting端(等待別人打電話過來的那一端), 持續響鈴
		emitSignal(SIGNAL_RINGREPLY);
	}
	if (connected)
	{
		// renyang - 若對方接受你的電話, 或是本地端接受對方電話, 則停止探查目前接收方的情況
		checkTimer->stop();
		// renyang - 送出訊息說對方接受電話啦
		emit connectedSignal();
	}
}

void Receiver::close()
{
#ifdef REN_DEBUG
	qWarning("Receiver::close()");
#endif
	if (notifier)
		delete notifier;
	notifier = NULL;
	
	s = -1;

	received = false;
}

// renayng - 結束傳送端的服務
void Receiver::end()
{
#ifdef REN_DEBUG
	qWarning("Receiver::end()");
#endif
	close();
	
	// renyang - 這一行是避免對方打電話過來, 我還沒有接電話, 對方馬上就把它掛掉了
	checkTimer->stop();
	
	stop();
	
	disableDecrypt();
}

// renyang-TODO - 加入IHU_SCTP的部分
// renyang - 當client端有傳送資料過來這裡時, 則會執行此函式
// renyang - 接收資料到這裡就算是結束了, 剩下的其它部分會處理
void Receiver::receive()
{
#ifdef REN_DEBUG
	qWarning("Receiver::receive()");
#endif
	// renyang - 哇!!竟然不用delete notifier???
	if (working)
	{
		int rlen = 0;
		
		switch(protocol)
		{
			// renyang - 把接收到的資料放到inputBuffer中
			case IHU_UDP:
				// renyang - 由udp的方式來接收資料
				// renyang - 限制資料一定是由ca這一個sockaddr傳送過來的
				rlen = ::recvfrom(s, inputBuffer, IN_BUFSIZE, 0, (struct sockaddr *)&ca, &calen);
				break;
			case IHU_TCP:
				// renyang - 由tcp的方式來接收資料
				// renyang - 這裡沒有用Qt的接收資料的方式
				// renyang - 還是用一般socket的使用方式
				rlen = ::recv(s, inputBuffer, IN_BUFSIZE, 0);
				break;
		}

		// renyang - 當接收字串長度大於0時, 表示有接收到資料
		if (rlen > 0)
		{
			bytes += rlen;
			total += rlen;
			active = true;
			putData(inputBuffer, rlen);
		}
		else
		{
			// renyang - 當有資料送過來, 但是長度確沒有大於0，表示對方出現錯誤或是離開
			emitSignal(SIGNAL_FINISH);
		}
	}
}

// renyang - 把接收到的資料放到streamBuffer中
void Receiver::putData(char *buffer, int len)
{
#ifdef REN_DEBUG
	qWarning(QString("Receiver::putData(char *buffer, int %1)").arg(len));
#endif
	// renyang - 應該是要把音訊檔存到檔案中吧
	if (outFile)
	{
		// renyang - 把接收到的資料放到outFile中
		fwrite(buffer, 1, len, outFile);
		fflush(outFile);
	}
	
	// renyang - 若剩下的buffer(MAXBUFSIZE - len)小於streamLen，則表示空間不夠, 出現錯誤啦當有資料送過來, 但是長度確沒有大於0，表示對方出現錯誤或是離開
	// renyang - streamLen可以看成MAXBUFSIZE目前使用到哪裡的index
	if (streamLen > (MAXBUFSIZE - len))
	{
		qWarning("Warning: receiver buffer overloaded.");
		// renyang - 把實際的資料刪成剛好可以放到buffer的大小
		len = MAXBUFSIZE - streamLen;
	}
	memcpy(streamBuffer + streamLen, buffer, len);
	streamLen += len;
	
	// renyang - 資料完成放到streamBuffer中啦
	if (sync == STREAM_READ_DATA)
	{
		sync = STREAM_OK;
		processData();
	}
}

void Receiver::processData()
{
#ifdef REN_DEBUG
	qWarning("Receiver::processData()");
#endif
	while (working && (sync != STREAM_READ_DATA))
	{
		switch (sync)
		{
			case STREAM_OK:
				// renyang - 至少要有一個資料, 否則就是資料遺失
				if (streamLen < (HEADER_SYNC_LEN + 1))
				{
					sync = STREAM_MISSING_DATA;
					break;
				}
				// renyang - 比較Header是否有問題
				if (strncmp(streamPtr, HEADER_SYNC_STRING, HEADER_SYNC_LEN) != 0)
				{
					sync = STREAM_OUT_OF_SYNC;
#ifdef REN_DEBUG
					fprintf(stderr, "OUT OF SYNC (Dump: 0x");
					for(int i=0;i<4;i++)
						fprintf(stderr, "%02x", (unsigned char) streamPtr[i]);
					fprintf(stderr, "\n");
#endif
					break;
				}
				else
				{
					// renyang - 實際封包的大小(內容值是由peer端送過來的)
					unsigned char packetlen = (unsigned char) streamPtr[HEADER_SYNC_LEN];
					int plen = (int) packetlen;
					// renyang - 封包面裡記錄的資料大小比streamLen還要大, 則表示有資料遺失
					if (plen > streamLen)
					{
						sync = STREAM_MISSING_DATA;
						break;
					}
					else
					{
						try {
							if (plen >= MIN_PACKET_SIZE)
							{
								Packet *p = new Packet(plen);
								// renyang - 建立Packet
								PacketHandler::readPacket(p, streamPtr, plen);
								processPacket(p);
								delete p;
							}
							else
							{
								sync = STREAM_OUT_OF_SYNC;
								break;
							}
							if (plen < streamLen)
							{
								streamPtr = streamPtr + plen;
								streamLen = streamLen - plen;
								sync = STREAM_OK;
							}
							else 
							{
								sync = STREAM_READ_DATA;
								resetStream();
							}
						} catch (Error e)
						{
							emitError(e.getText());
						}
					}
				}
				break;
			case STREAM_OUT_OF_SYNC:
				if (streamLen < 3)
					sync = STREAM_MISSING_DATA;
				else
				{
					do
					{
						streamPtr++;
						streamLen--;
						if (streamLen <= 0)
						{
							sync = STREAM_READ_DATA;
							resetStream();
							break;
						}
						if (strncmp(streamPtr, HEADER_SYNC_STRING, HEADER_SYNC_LEN)==0)
						{
							sync = STREAM_OK;
						}
					} while(sync == STREAM_OUT_OF_SYNC);
				}
				break;
			case STREAM_MISSING_DATA:
				sync = STREAM_READ_DATA;
			case STREAM_READ_DATA:
				memmove(streamBuffer, streamPtr, streamLen);
				streamPtr = streamBuffer;
				break;
		}
	}
}

// renyang-TODO - 加入Vedio的部分吧
bool Receiver::processPacket(Packet *p)
{
#ifdef REN_DEBUG
	qWarning("Receiver::processPacket(Packet *p)");
#endif
	switch (p->getInfo())
	{
		case IHU_INFO_CRYPTED_AUDIO:
			if (blowfish)
			{
				// renyang - 解碼
				p->decrypt(blowfish);
			}
			else
			{
				if (!nodecrypt)
				{
					emitSignal(SIGNAL_KEYREQUEST);
				}
				break;
			}
		case IHU_INFO_AUDIO:
			// renyang - 取出資料, 並且通知有新資料到
			if (p->getDataLen() > MIN_DATA_SIZE)
			{
				emit newAudioData(p->getData(), p->getDataLen());
			}
			connected = true;
			break;
		case IHU_INFO_NEW_KEY:
			if (p->getDataLen() > MIN_DATA_SIZE)
			{
				char *out;
				int len = rsa->decrypt(p->getData(), p->getDataLen(), &out);
				if (blowfish)
					delete blowfish;
				blowfish = new Blowfish(out, len);
				emitSignal(SIGNAL_NEWKEY);
				free(out);
			}
			break;
		case IHU_INFO_KEY_REQUEST:
			if (p->getDataLen() > MIN_DATA_SIZE)
			{
				rsa->setPeerPublicKey(p->getData(), p->getDataLen());
				emitSignal(SIGNAL_SENDNEWKEY);
			}
			break;
		case IHU_INFO_RING:
			if (p->getDataLen() > MIN_DATA_SIZE)
			{
				ihu_reply = true;
				QString tempName = p->getData(); 
				if (!tempName.isEmpty())
					callerName = tempName;
			}
			emit warning(QString("!! CALL from %1 (%2) !!").arg(getIp()).arg(getCallerName()));
			emitSignal(SIGNAL_RING);
			break;
		case IHU_INFO_ANSWER:
			connected = true;
		case IHU_INFO_RING_REPLY:
			ihu_reply = true;
			if (p->getDataLen() > MIN_DATA_SIZE)
				callerName = p->getData();
			break;
		case IHU_INFO_ERROR:
			ihu_abort = true;
		case IHU_INFO_REFUSE:
			ihu_refuse = true;
		case IHU_INFO_CLOSE:
			emitSignal(SIGNAL_FINISH);
			break;
		case IHU_INFO_INIT:
			emitSignal(SIGNAL_INIT);
		case IHU_INFO_RESET:
			disableDecrypt();
			break;
	}
	return true;
}

void Receiver::emitError(QString text)
{
#ifdef REN_DEBUG
	qWarning(QString("Receiver::emitError(QString %1)").arg(text));
#endif
	emit error(text);
}

void Receiver::enableDecrypt(char *passwd, int len)
{
#ifdef REN_DEBUG
	qWarning(QString("Receiver::enableDecrypt(char %1, int %2)").arg(passwd).arg(len));
#endif
	disableDecrypt();
	blowfish = new Blowfish(passwd, len);
}

// renyang - 加解密的部分
void Receiver::disableDecrypt()
{
#ifdef REN_DEBUG
	qWarning("Receiver::disableDecrypt()");
#endif
	if (blowfish)
		delete blowfish;
	blowfish = NULL;
}

void Receiver::stop()
{
#ifdef REN_DEBUG
	qWarning("Receiver::stop()");
#endif
	working = false;
}

void Receiver::go()
{
#ifdef REN_DEBUG
	qWarning("Receiver::go()");
#endif
	working = true;
}

long Receiver::getBytes()
{
#ifdef REN_DEBUG
	qWarning("Receiver::getBytes()");
#endif
	long temp = bytes;
	bytes = 0;
	return temp;
}

long Receiver::getTotal()
{
#ifdef REN_DEBUG
	qWarning("Receiver::getTotal()");
#endif
	return total;
}

QString Receiver::getIp()
{
#ifdef REN_DEBUG
	qWarning("Receiver::getIp()");
#endif
	return QString(inet_ntoa(ca.sin_addr));
}

QString Receiver::getCallerName()
{
#ifdef REN_DEBUG
	qWarning("Receiver::getCallerName()");
#endif
	return callerName;
}

void Receiver::noDecrypt()
{
#ifdef REN_DEBUG
	qWarning("Receiver::noDecrypt()");
#endif
	nodecrypt = true;
}

bool Receiver::refused()
{
#ifdef REN_DEBUG
	qWarning("Receiver::refused()");
#endif
	return ihu_refuse;
}

bool Receiver::aborted()
{
#ifdef REN_DEBUG
	qWarning("Receiver::aborted()");
#endif
	return ihu_abort;
}

bool Receiver::replied()
{
#ifdef REN_DEBUG
	qWarning("Receiver::replied()");
#endif
	return ihu_reply;
}

// renyang - 只是用來判斷完成某件事情的訊號
void Receiver::emitSignal(signal_type type)
{
#ifdef REN_DEBUG
	qWarning(QString("Receiver::emitSignal(signal_type %1)").arg(type));
#endif
	switch(type)
	{
		case SIGNAL_FINISH:
			emit finishSignal();
			break;
		if (working)
		{
			case SIGNAL_INIT:
				emit initSignal();
				break;
			case SIGNAL_RING:
				emit ringSignal();
				break;
			// renyang - 回覆答鈴
			case SIGNAL_RINGREPLY:
				emit ringReplySignal();
				break;
			case SIGNAL_SENDNEWKEY:
				emit sendNewKey();
				break;
			case SIGNAL_KEYREQUEST:
				emit keyRequest();
				break;
			case SIGNAL_NEWKEY:
				QString text = blowfish->isAsciiKey() ? blowfish->getPass() : "random key";
				emit newKey(text);
				break;
		}
		default:
			break;
	}
}

void Receiver::flush()
{
#ifdef REN_DEBUG
	qWarning("Receiver::flush()");
#endif
	stop();
	flushing = true;
}

bool Receiver::isDumping()
{
#ifdef REN_DEBUG
	qWarning("Receiver::isDumping()");
#endif
	bool ret = false;
	if (outFile)
		ret = true;
	return ret;
}

void Receiver::setConnected(bool on)
{
#ifdef REN_DEBUG
	qWarning(QString("Receiver::setConnected(bool %1)").arg(on));
#endif
	connected = on;
}

bool Receiver::isConnected()
{
#ifdef REN_DEBUG
	qWarning("Receiver::isConnected()");
#endif
	return connected;
}

// renyang - 設定已接收client端的連線(就網路的部分, 還沒有接受通話)
void Receiver::setReceived(bool on)
{
#ifdef REN_DEBUG
	qWarning(QString("Receiver::setReceived(bool %1)").arg(on));
#endif
	received = on;
}

bool Receiver::isReceived()
{
#ifdef REN_DEBUG
	qWarning("Receiver::isReceived()");
#endif
	return received;
}

bool Receiver::isActive()
{
#ifdef REN_DEBUG
	qWarning("Receiver::isActive()");
#endif
	bool temp = active;
	active = false;
	return temp;
}
