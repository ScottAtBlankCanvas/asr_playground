/*
 * Copyright (c) 2025, Scott Kellicker. All rights reserved.
 *
 *
 *
 *
 * This code originated from examples in (the excellent) websocketpp project, including client_debug.cpp, echo_client.cpp and others
 *
 * https://github.com/zaphoyd/websocketpp
 *
 * The websocketpp examples contain the following license information:
 *
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

using std::chrono::duration;
using std::chrono::duration_cast;
using std::cout;
using std::endl;
using std::exception;
using std::ifstream;
using std::string;
using std::vector;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;  // use wss:
typedef client::connection_ptr connection;
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;


class speechmatics_test {
public:
    typedef speechmatics_test type;

    speechmatics_test (string test_file) {

        this->m_file_path = test_file;

        init_logging(); 
        init_network(); 
        init_websocket_handlers();
    }

    void init_logging() {
        m_websocket.set_access_channels(websocketpp::log::alevel::fail);
        m_websocket.set_error_channels(websocketpp::log::elevel::none);
        m_websocket.get_alog().clear_channels(websocketpp::log::alevel::frame_header |
        websocketpp::log::alevel::frame_payload |
        websocketpp::log::alevel::control);
    }

    void init_network() {
        m_websocket.init_asio();
    }
    void init_websocket_handlers() {
        m_websocket.set_socket_init_handler(bind(&type::on_socket_init,this,::_1));
        m_websocket.set_tls_init_handler(bind(&type::on_tls_init,this,::_1));
        m_websocket.set_message_handler(bind(&type::on_message,this,::_1,::_2));
        m_websocket.set_open_handler(bind(&type::on_open,this,::_1));
        m_websocket.set_ping_handler(bind(&type::on_ping,this,::_1,::_2));
        m_websocket.set_pong_handler(bind(&type::on_pong,this,::_1,::_2));
        m_websocket.set_pong_timeout_handler(bind(&type::on_pong_timeout,this,::_1,::_2));
        m_websocket.set_close_handler(bind(&type::on_close,this,::_1));
        m_websocket.set_fail_handler(bind(&type::on_fail,this,::_1));
        m_websocket.set_interrupt_handler(bind(&type::on_interrupt,this,::_1));
    }

    void start(string uri, string api_key) {
        cout << ">> start url: " << uri << " key: " << api_key << endl;
        websocketpp::lib::error_code ec;
        client::connection con = m_websocket.get_connection(uri, ec);

        if (ec) {
            cout << "   error        : " << ec << " - " << ec.message() << endl;
            return;
        }

        // Add the Authorization header with a Bearer token
        con->replace_header("Authorization", "Bearer " + api_key);

        m_websocket.connect(con);

        // Start the ASIO io_service run loop
        m_websocket.run();
    }

    void close(websocketpp::connection_hdl hdl) {
        cout << ">> close" << endl;

        m_websocket.close(hdl,websocketpp::close::status::going_away,"");

        cout << endl;
    }
    void send_text_message(websocketpp::connection_hdl hdl, string m) {
        cout << ">> send_text_message: " << m << endl;

        m_websocket.send(hdl, m, websocketpp::frame::opcode::text);

        cout << endl;
    }

    void send_binary_data(websocketpp::connection_hdl hdl, vector<char> vec, int len = 0) {
        // Entire buffer
        if (len == 0) len = vec.size();

        const void* data = static_cast<const void*>(vec.data());
        m_websocket.send(hdl, data, len, websocketpp::frame::opcode::binary);
    }

    void on_socket_init(websocketpp::connection_hdl) {
        cout << "<< on_socket_init" << endl;
    }

    context_ptr on_tls_init(websocketpp::connection_hdl) {
        cout << "<< on_tls_init" << endl;
        context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

        try {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3 |
                             boost::asio::ssl::context::single_dh_use);
        } catch (exception& e) {
            cout << e.what() << endl;
        }

        return ctx;
    }


    void on_fail(websocketpp::connection_hdl hdl) {
        client::connection con = m_websocket.get_con_from_hdl(hdl);

        cout << "<< on_fail" << endl;
        cout << "   state        : " << con->get_state() << endl;
        cout << "   local_code   : " << con->get_local_close_code() << endl;
        cout << "   local_reason : " << con->get_local_close_reason() << endl;
        cout << "   remote_code  : " << con->get_remote_close_code() << endl;
        cout << "   remote_reason: " << con->get_remote_close_reason() << endl;
        cout << "   error        : " << con->get_ec() << " - " << con->get_ec().message() << endl;

        m_running = false;

        cout << endl;
    }

    void on_interrupt(websocketpp::connection_hdl hdl) {
        client::connection con = m_websocket.get_con_from_hdl(hdl);

        cout << "<< on_interrupt" << endl;
        cout << "   state        : " << con->get_state() << endl;
        cout << "   local_code   : " << con->get_local_close_code() << endl;
        cout << "   local_reason : " << con->get_local_close_reason() << endl;
        cout << "   remote_code  : " << con->get_remote_close_code() << endl;
        cout << "   remote_reason: " << con->get_remote_close_reason() << endl;
        cout << "   error        : " << con->get_ec() << " - " << con->get_ec().message() << endl;

        m_running = false;
    }

    void on_open(websocketpp::connection_hdl hdl) {
        cout << "<< on_open" << endl;

        m_running = true;

        cout << endl;


        speechmatics_start_recognition(hdl);
    }

    void on_message(websocketpp::connection_hdl hdl, message_ptr msg) {

        // Ignore some messages
        if (msg->get_payload().find("AudioAdded") != string::npos) {
        } 
        else {
            cout << "<< on_message: " << msg->get_payload() << endl << endl;

            if (msg->get_payload().find("RecognitionStarted") != string::npos) {
                speechmatics_stream_wav_file(hdl);
            }

        }
    }

    void on_close(websocketpp::connection_hdl) {
        cout << "<< on_close" << endl;

        m_running = false;
    }

    bool on_ping(websocketpp::connection_hdl hdl, string msg) {
        return true;
    }
    bool on_pong(websocketpp::connection_hdl hdl, string msg) {
        return true;
    }
    bool on_pong_timeout(websocketpp::connection_hdl hdl, string msg) {
        return true;
    }


    void speechmatics_start_recognition(websocketpp::connection_hdl hdl) {
        cout << ">> speechmatics_start_recognition" << endl;

        string m = 
        "{ "
            " \"message\": \"StartRecognition\", "
            " \"audio_format\": {  \"type\": \"raw\",  \"encoding\": \"pcm_s16le\",  \"sample_rate\": 16000 }, "
            " \"transcription_config\": {  \"language\": \"en\" }"
        "}";
        cout << "   " << m << endl;
        cout << endl;

        send_text_message(hdl, m);
    }

    // This is a bit hacky bc we should send audio data in real time and in a separate thread
    // But works OK for verifying Speechmatics API
    void speechmatics_stream_wav_file(websocketpp::connection_hdl hdl) {
       int BUFFER_SIZE = 1024;
       vector<char> buffer(BUFFER_SIZE);

       ifstream infile(m_file_path, std::ios::binary);
       if (!infile.is_open()) {
           cout << "   Error opening file: " << m_file_path << endl;
       return;
       }
       cout << ">> speechmatics_stream_wav_file: " << m_file_path << endl;

       // true as long as read fills buffer
       while (infile.read(buffer.data(), BUFFER_SIZE)) {
           send_binary_data(hdl, buffer);
       }

       // What's left in buffer 
       if (infile.gcount() > 0) {
           send_binary_data(hdl, buffer, infile.gcount());
       }

       infile.close();

       cout << endl;
    }

private:
    client m_websocket;
    bool m_running = false;
    string m_file_path = "speech_test.wav";
};

int main(int argc, char* argv[]) {
    string uri = "wss://wus.rt.speechmatics.com/v2";
    string api_key = "Unset API Token";
    string test_file = "speech_test.wav";

    if (argc >= 2) uri = argv[1];
    if (argc >= 3) api_key = argv[2];
    if (argc >= 4) test_file = argv[3];

    cout << "Testing Speechmatics streaming" << endl;
    cout << "  uri : " << uri << endl;
    cout << "  api : " << api_key.substr(0,6) << "*" << endl;
    cout << "  file: " << test_file << endl;

    try {
        speechmatics_test endpoint(test_file);

        endpoint.start(uri, api_key);

    } catch (websocketpp::exception const & e) {
        cout << e.what() << endl;
    } catch (exception const & e) {
        cout << e.what() << endl;
    } catch (...) {
        cout << "other exception" << endl;
    }
}
