/* Simple Chat
 * Copyright (c) 2008-2015 Alexander Sedov <imp@schat.me>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QUuid>

#include "AuthProxy.h"
#include "Ch.h"
#include "Core.h"
#include "net/packets/auth.h"
#include "NodeAuth.h"
#include "NodeLog.h"
#include "sglobal.h"
#include "SJMPPacket.h"
#include "tools/Ver.h"

AuthProxy::AuthProxy(const AuthRequest &data, const QString &ip, QObject *parent)
  : QObject(parent)
  , m_data(data)
  , m_id(ChatId(data.id).toString())
  , m_ip(ip)
  , m_uuid(QUuid::createUuid().toString().mid(1, 36))
  , m_socket(Core::socket())
{
  connect(Core::i(), SIGNAL(packet(SJMPPacket)), SLOT(onPacket(SJMPPacket)));

  Core::send(request());
}


void AuthProxy::onPacket(const SJMPPacket &packet)
{
  if (packet.id() != m_id)
    return;

  deleteLater();

  if (packet.status() != 200) {
    int status = Notice::Forbidden;
    if (packet.status() == 302)
      status = Notice::Found;

    return Core::i()->reject(m_data, AuthResult(status, m_data.id), m_socket);
  }

  const QVariantMap profile = packet.body().toMap().value(LS("user")).toMap();
  const ChatId chatId(profile.value(LS("chatId")).toString());

  Q_ASSERT(chatId.type() == ChatId::UserId);
  if (chatId.type() != ChatId::UserId) {
    return Core::i()->reject(m_data, AuthResult(Notice::Forbidden, m_data.id), m_socket);
  }

  m_data.authType = AuthRequest::Cookie;
  m_data.cookie   = ChatId(packet.header(LS("cookie")).toString()).toByteArray();

  ChatChannel channel = Ch::channel(chatId.toByteArray(), SimpleID::UserId);

  if (!channel) {
    channel = ChatChannel(new ServerChannel(chatId.toByteArray(), profile.value(LS("nick")).toString()));
  }

  channel->setName(profile.value(LS("nick")).toString());
  channel->gender().set(profile);

  if (channel->status() == Status::Offline) {
    channel->setDate();
    channel->setStatus(profile.value(LS("status")).toInt());
  }

  channel->setData(LS("profile"), profile);

  Core::add(channel, m_uuid, request());
  Ch::userChannel(channel, m_data, packet, m_ip, m_uuid, m_socket);

  LOG_INFO("N1100", "Core/AuthProxy", channel->name() << "@" << m_ip + "/" + chatId.toString() << ", " << m_data.host)

  const AuthResult result(chatId.toByteArray(), m_data.id, packet);
  Core::i()->accept(m_data, result, m_ip);
}


SJMPPacket AuthProxy::request() const
{
  SJMPPacket packet;
  packet.setMethod(LS("post"));
  packet.setResource(LS("v4/auth"));
  packet.setId(m_id);
  packet.setHeader(LS("ip"), m_ip);
  packet.setHeader(LS("uuid"), m_uuid);
  packet.setHeader(LS("ua"), QString("(%1) Simple Chat/%2").arg(m_data.json.value(LS("os")).toString()).arg(Ver(m_data.version).toString()));
  packet.setBody(m_data.toJSON());

  return packet;
}
