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
typedef client::connection_ptr connection_ptr;
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;


class options {
public:
    options(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
//        cout << "Argument " << i << ": " << arg << endl;

            if (arg == "--help" ) {
	        show_usage(argv[0]);
	        exit(0);
            } else if (arg == "--uri" && i + 1 < argc) {
                this->uri = argv[++i];
                    cout << "uri     : " << this->uri << endl;
            } else if (arg == "--file" && i + 1 < argc) {
                this->test_file = argv[++i];
                cout << "file    : " << this->test_file << endl;
            } else if (arg == "--key" && i + 1 < argc) {
                this->api_key = argv[++i];
                cout << "key     : " << this->api_key << endl;
            } else if (arg == "--partials" && i + 1 < argc) {
                this->partials = argv[++i];
                cout << "partials: " << this->partials << endl;
            } else if (arg == "--entities" && i + 1 < argc) {
                this->entities = argv[++i];
                cout << "entities: " << this->entities << endl;
            } else if (arg == "--model" && i + 1 < argc) {
                this->model = argv[++i];
                cout << "model   : " << this->model << endl;
            } else if (arg == "--lang" && i + 1 < argc) {
                this->lang = argv[++i];
                cout << "lang    : " << this->lang << endl;
            }
	}
    }
private:
    void show_usage(char* pgm) {
        cout << "Usage: " << pgm << " --key <api_key> --file <filename> [optional] " << endl;
        cout << "  optional:" << endl;
        cout << "  --help            : this message" << endl;
        cout << endl;
        cout << "  --lang            : Input language (def: en)" << endl;
        cout << "  --uri             : Speechmatics endpoint " << endl;
        cout << "  --partials        : Enable partials (def: false)" << endl;
        cout << "  --entities        : Enable entities (def: false)" << endl;
        cout << "  --model           : Operating point standard|enhanced (def: standard)" << endl;

        exit(0);
    }

public:
    string uri       = "wss://wus.rt.speechmatics.com/v2";
    string api_key   = "Unset API Token";
    string test_file = "speech_test.wav";
    string lang      = "en";
    string partials  = "false";
    string entities  = "false";
    string model     = "standard";
};


class speechmatics_test {
public:
    typedef speechmatics_test type;

    speechmatics_test (const options& opts) : m_opts(opts) {

//        this->m_opts = opts;

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

    void start() {
        cout << ">> start url: " << m_opts.uri << " key: " << m_opts.api_key << endl;
        websocketpp::lib::error_code ec;
        connection_ptr con = m_websocket.get_connection(m_opts.uri, ec);

        if (ec) {
            cout << "   error        : " << ec << " - " << ec.message() << endl;
            return;
        }

        // Add the Authorization header with a Bearer token
        con->replace_header("Authorization", "Bearer " + m_opts.api_key);

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
        connection_ptr con = m_websocket.get_con_from_hdl(hdl);

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
        connection_ptr con = m_websocket.get_con_from_hdl(hdl);

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
            " \"transcription_config\": {  \"language\": \"" + m_opts.lang + "\", \"operating_point\": \"" + m_opts.model + "\", \"enable_partials\": "+ m_opts.partials + ", \"enable_entities\":"+m_opts.entities+" }"
        "}";
        cout << "   " << m << endl;
        cout << endl;

        send_text_message(hdl, m);
    }

    // This is a bit hacky bc we should send audio data in real time and in a separate thread
    // But works OK for verifying Speechmatics API
    void speechmatics_stream_wav_file(websocketpp::connection_hdl hdl) {
       const int MAX_SENDS = 500;
       const int BUFFER_SIZE = 1024;
       vector<char> buffer(BUFFER_SIZE);
       int count = 0;

       ifstream infile(m_opts.test_file, std::ios::binary);
       if (!infile.is_open()) {
           cout << "   Error opening file: " << m_opts.test_file << endl;
	   exit(-1);
       }
       cout << ">> speechmatics_stream_wav_file: " << m_opts.test_file << endl;

       // true as long as read fills buffer
       while (infile.read(buffer.data(), BUFFER_SIZE)) {
           if (count++ > MAX_SENDS) goto do_exit;
           send_binary_data(hdl, buffer);
       }

       // What's left in buffer 
       if (infile.gcount() > 0) {
           send_binary_data(hdl, buffer, infile.gcount());
       }

       do_exit:
              infile.close();

              cout << endl;
    }

private:
    client m_websocket;
    bool m_running = false;
    const options &m_opts;
};

int main(int argc, char* argv[]) {

    try {
        options opts(argc, argv);

        speechmatics_test endpoint(opts);

        endpoint.start();

    } catch (websocketpp::exception const & e) {
        cout << e.what() << endl;
    } catch (exception const & e) {
        cout << e.what() << endl;
    } catch (...) {
        cout << "other exception" << endl;
    }
}
