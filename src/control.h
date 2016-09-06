/*
*  Copyright 2016 The PeerConnect Project Authors. All rights reserved.
*
*  Ryan Lee
*/

#ifndef __PEERCONNECT_CONTROL_H__
#define __PEERCONNECT_CONTROL_H__

#include <memory>

#include "peer.h"
#include "signalconnection.h"
#include "controlobserver.h"

#include "webrtc/base/sigslot.h"
#include "fakeaudiocapturemodule.h"


namespace pc {

class Control
    : public PeerObserver,
      public sigslot::has_slots<>,
      public rtc::MessageHandler {
public:

  using string = std::string;
  using DataChannelList = std::vector<std::unique_ptr<PeerDataChannelObserver> >;

  explicit Control();
  explicit Control(std::shared_ptr<Signal> signal);
  virtual ~Control();

  //
  // Initialize and release
  //

  bool InitializeControl();
  void DeleteControl();
  
  //
  // Negotiation and send data
  //

  void Send(const string to, const char* buffer, const size_t size);
  bool SyncSend(const string to, const char* buffer, const size_t size);

  void Open(const string& user_id, const string& user_password, const string& channel);
  void Close(const CloseCode code, bool force_queueing = FORCE_QUEUING_OFF);
  void Connect(const string channel);
  bool IsWritable(const string channel);

  void OnCommandReceived(const Json::Value& message);
  void OnSignalCommandReceived(const Json::Value& message);
  void OnSignalConnectionClosed(websocketpp::close::status::value code);

  //
  // PeerObserver implementation
  //

  virtual void SendCommand(const string& channel, const string& command, const Json::Value& data);
  virtual void ClosePeer( const string channel, const CloseCode code,  bool force_queueing = FORCE_QUEUING_OFF );
  virtual void OnPeerConnect(const string channel);
  virtual void OnPeerClose(const string channel, const CloseCode code);
  virtual void OnPeerMessage(const string& channel, const char* buffer, const size_t size);
  virtual void OnPeerWritable(const string& channel);


  // Register/Unregister observer
  void RegisterObserver(ControlObserver* observer, std::shared_ptr<Control> ref);
  void UnregisterObserver();

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg);

protected:
  void CreateChannel(const string name);
  void JoinChannel(const string name);
  void LeaveChannel(const string name);
  bool CreatePeerFactory(const webrtc::MediaConstraintsInterface* constraints);
  void CreateOffer(const Json::Value& data);
  void AddIceCandidate(const string& peer_id, const Json::Value& data);
  void ReceiveOfferSdp(const string& peer_id, const Json::Value& data);
  void ReceiveAnswerSdp(const string& peer_id, const Json::Value& data);

  void OnOpen(const Json::Value& data);
  void OnChannelCreate(const Json::Value& data);
  void OnChannelJoin(const Json::Value& data);
  void OnChannelLeave(const Json::Value& data);
  void OnRemotePeerClose(const string& peer_id, const Json::Value& data);


  // channel_: A name of local channel. Other peers can find this peer by channel_
  // user_id_: A user id to sign in signal server (could be 'anonymous' for guest user)
  // session_id_: A unique id for signal server connection
  string channel_;
  string user_id_;
  string session_id_;

  std::shared_ptr<Signal> signal_;
  rtc::scoped_refptr<FakeAudioCaptureModule> fake_audio_capture_module_;

  using Peer = rtc::scoped_refptr<PeerControl>;
  std::map<string, Peer> peers_;

  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;

private:

  enum {
    MSG_COMMAND_RECEIVED,           // Command has been received from signal server
    MSG_CLOSE,                      // Queue signout request
    MSG_CLOSE_PEER,                 // Close peer
    MSG_ON_PEER_CLOSE,              // Peer has been closed
    MSG_ON_SIGLAL_CONNECTION_CLOSE  // Connection to signal server has been closed
  };

  struct ControlMessageData : public rtc::MessageData {
    explicit ControlMessageData(Json::Value data, std::shared_ptr<Control> ref) : data_json_(data), ref_(ref) {}
    explicit ControlMessageData(const string data, std::shared_ptr<Control> ref) : data_string_(data), ref_(ref) {}
    explicit ControlMessageData(const uint32_t data, std::shared_ptr<Control> ref) : data_int32_(data), ref_(ref) {}
    Json::Value data_json_;
    string data_string_;
    uint32_t data_int32_;

  private:
    std::shared_ptr<Control> ref_;
  };

  rtc::Thread* webrtc_thread_;
  ControlObserver* pc_;
  std::shared_ptr<Control> ref_;
};

} // namespace pc

#endif // __PEERCONNECT_CONTROL_H__
