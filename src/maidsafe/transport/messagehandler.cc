/* Copyright (c) 2010 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <boost/lexical_cast.hpp>
#include "maidsafe/transport/messagehandler.h"
#include "maidsafe/transport/transport.pb.h"
#include "maidsafe/common/securifier.h"

namespace maidsafe {

namespace transport {

enum MessageType {
  kManagedEndpointMessage = 1,
  kNatDetectionRequest,
  kNatDetectionResponse,
  kProxyConnectRequest,
  kProxyConnectResponse,
  kForwardRendezvousRequest,
  kForwardRendezvousResponse,
  kRendezvousRequest,
  kRendezvousAcknowledgement
};

void MessageHandler::OnMessageReceived(const std::string &request,
                                       const Info &info,
                                       std::string *response,
                                       Timeout *timeout) {
  protobuf::WrapperMessage wrapper;

  // Try to parse without decrypting first
  if (wrapper.ParseFromString(request) && wrapper.IsInitialized()) {
    return ProcessSerialisedMessage(wrapper.msg_type(), wrapper.payload(),
                                    wrapper.message_signature(), info, false,
                                    response, timeout);
  } else {  // Now try decrypting
    if (!securifier_)
      return;
    std::string decrypted(securifier_->AsymmetricDecrypt(request));
    if (!wrapper.ParseFromString(decrypted))
      return;
    if (wrapper.IsInitialized())
      return ProcessSerialisedMessage(wrapper.msg_type(), wrapper.payload(),
                                      wrapper.message_signature(), info, true,
                                      response, timeout);
  }
}

void MessageHandler::OnError(const TransportCondition &transport_condition) {
  (*on_error_)(transport_condition);
}

std::string MessageHandler::WrapMessage(
    const protobuf::ManagedEndpointMessage &msg) {
  return MakeSerialisedWrapperMessage(kManagedEndpointMessage,
                                      msg.SerializeAsString(), kNone);
}

std::string MessageHandler::WrapMessage(
    const protobuf::NatDetectionRequest &msg) {
  return MakeSerialisedWrapperMessage(kNatDetectionRequest,
                                      msg.SerializeAsString(), kNone);
}

std::string MessageHandler::WrapMessage(
    const protobuf::NatDetectionResponse &msg) {
  return MakeSerialisedWrapperMessage(kNatDetectionResponse,
                                      msg.SerializeAsString(), kNone);
}

std::string MessageHandler::WrapMessage(
    const protobuf::ProxyConnectRequest &msg) {
  return MakeSerialisedWrapperMessage(kProxyConnectRequest,
                                      msg.SerializeAsString(), kNone);
}

std::string MessageHandler::WrapMessage(
    const protobuf::ProxyConnectResponse &msg) {
  return MakeSerialisedWrapperMessage(kProxyConnectResponse,
                                      msg.SerializeAsString(), kNone);
}

std::string MessageHandler::WrapMessage(
    const protobuf::ForwardRendezvousRequest &msg) {
  return MakeSerialisedWrapperMessage(kForwardRendezvousRequest,
                                      msg.SerializeAsString(), kNone);
}

std::string MessageHandler::WrapMessage(
    const protobuf::ForwardRendezvousResponse &msg) {
  return MakeSerialisedWrapperMessage(kForwardRendezvousResponse,
                                      msg.SerializeAsString(), kNone);
}

std::string MessageHandler::WrapMessage(
    const protobuf::RendezvousRequest &msg) {
  return MakeSerialisedWrapperMessage(kRendezvousRequest,
                                      msg.SerializeAsString(), kNone);
}

std::string MessageHandler::WrapMessage(
    const protobuf::RendezvousAcknowledgement &msg) {
  return MakeSerialisedWrapperMessage(kRendezvousAcknowledgement,
                                      msg.SerializeAsString(), kNone);
}

void MessageHandler::ProcessSerialisedMessage(
    const int &message_type,
    const std::string &payload,
    const std::string &/*message_signature*/,
    const Info &/*info*/,
    bool /*asymmetrical_encrypted*/,
    std::string *message_response,
    Timeout *timeout) {
  message_response->clear();
  *timeout = kImmediateTimeout;

  switch (message_type) {
    case kManagedEndpointMessage: {
      protobuf::ManagedEndpointMessage request;
      if (request.ParseFromString(payload) && request.IsInitialized()) {
        protobuf::ManagedEndpointMessage response;
        (*on_managed_endpoint_message_)(request, &response);
        if (!(*message_response = WrapMessage(response)).empty())
          *timeout = kDefaultInitialTimeout;
      }
      break;
    }
    case kNatDetectionRequest: {
      protobuf::NatDetectionRequest request;
      if (request.ParseFromString(payload) && request.IsInitialized()) {
//         NatDetectionReqSigPtr::element_type::result_type
//             (NatDetectionReqSigPtr::element_type::*sig)
//             (NatDetectionReqSigPtr::element_type::arg<0>::type,
//              NatDetectionReqSigPtr::element_type::arg<1>::type) =
//             &NatDetectionReqSigPtr::element_type::operator();
//         asio_service_->post(boost::bind(sig, on_nat_detection_, request,
//                                         conversation_id));
        protobuf::NatDetectionResponse response;
        (*on_nat_detection_request_)(request, &response);
        if (!(*message_response = WrapMessage(response)).empty())
          *timeout = kDefaultInitialTimeout;
      }
      break;
    }
    case kNatDetectionResponse: {
      protobuf::NatDetectionResponse response;
      if (response.ParseFromString(payload) && response.IsInitialized())
        (*on_nat_detection_response_)(response);
      break;
    }
    case kProxyConnectRequest: {
      protobuf::ProxyConnectRequest request;
      if (request.ParseFromString(payload) && request.IsInitialized()) {
        protobuf::ProxyConnectResponse response;
        (*on_proxy_connect_request_)(request, &response);
        if (!(*message_response = WrapMessage(response)).empty())
          *timeout = kDefaultInitialTimeout;
      }
      break;
    }
    case kProxyConnectResponse: {
      protobuf::ProxyConnectResponse response;
      if (response.ParseFromString(payload) && response.IsInitialized())
        (*on_proxy_connect_response_)(response);
      break;
    }
    case kForwardRendezvousRequest: {
      protobuf::ForwardRendezvousRequest request;
      if (request.ParseFromString(payload) && request.IsInitialized()) {
        protobuf::ForwardRendezvousResponse response;
        (*on_forward_rendezvous_request_)(request, &response);
        if (!(*message_response = WrapMessage(response)).empty())
          *timeout = kDefaultInitialTimeout;
      }
      break;
    }
    case kForwardRendezvousResponse: {
      protobuf::ForwardRendezvousResponse response;
      if (response.ParseFromString(payload) && response.IsInitialized())
        (*on_forward_rendezvous_response_)(response);
      break;
    }
    case kRendezvousRequest: {
      protobuf::RendezvousRequest request;
      if (request.ParseFromString(payload) && request.IsInitialized())
        (*on_rendezvous_request_)(request);
      break;
    }
    case kRendezvousAcknowledgement: {
      protobuf::RendezvousAcknowledgement acknowledgement;
      if (acknowledgement.ParseFromString(payload) &&
          acknowledgement.IsInitialized())
        (*on_rendezvous_acknowledgement_)(acknowledgement);
      break;
    }
  }
}

std::string MessageHandler::MakeSerialisedWrapperMessage(
    const int &message_type,
    const std::string &payload,
    SecurityType security_type) {
  protobuf::WrapperMessage wrapper_message;
  wrapper_message.set_msg_type(message_type);
  wrapper_message.set_payload(payload);

  // If we asked for security but provided no securifier, fail.
  if (security_type && !securifier_)
    return "";

  // Handle signing
  if (security_type & kSign) {
    wrapper_message.set_message_signature(securifier_->Sign(
        boost::lexical_cast<std::string>(message_type) + payload));
  } else if (security_type & kSignWithParameters) {
    wrapper_message.set_message_signature(securifier_->SignWithParameters(
        boost::lexical_cast<std::string>(message_type) + payload));
  }

  // Handle encryption
  if (security_type & kAsymmetricEncrypt) {
    return securifier_->AsymmetricEncrypt(wrapper_message.SerializeAsString());
  } else {
    return wrapper_message.SerializeAsString();
  }
}

}  // namespace transport

}  // namespace maidsafe
