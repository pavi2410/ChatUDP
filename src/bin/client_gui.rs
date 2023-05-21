use iced::{Alignment, Element, Length, Sandbox, Settings};
use iced::widget::{button, column, row, scrollable, text, text_input};
use iced::widget::scrollable::Properties;
use iced::window::Position::Centered;

pub fn main() -> iced::Result {
    Screens::run(Settings {
        window: iced::window::Settings {
            size: (800, 600),
            position: Centered,
            ..iced::window::Settings::default()
        },
        ..Settings::default()
    })
}

enum Screens {
    LoginScreen(LoginScreenState),
    ChatScreen(ChatScreenState),
}

struct LoginScreenState {
    username: String,
}

struct ChatScreenState {
    username: String,
    msg_input: String,
    users: Vec<String>,
    messages: Vec<ChatMsg>,
}

struct ChatMsg {
    username: String,
    msg: String,
}

#[derive(Debug, Clone)]
enum Message {
    Login,
    Username(String),
    MsgInput(String),
    Send,
}

impl Sandbox for Screens {
    type Message = Message;

    fn new() -> Self {
        Self::LoginScreen(LoginScreenState {
            username: "".to_string(),
        })
    }

    fn title(&self) -> String {
        let screen_name = match self {
            Screens::LoginScreen(_) => "Login",
            Screens::ChatScreen(_) => "Chat"
        };
        String::from(format!("ChatUDP - {}", screen_name))
    }

    fn update(&mut self, message: Message) {
        match self {
            Screens::LoginScreen(state) => {
                match message {
                    Message::Username(username) => {
                        state.username = username;
                    }
                    Message::Login => {
                        *self = Screens::ChatScreen(ChatScreenState {
                            username: state.username.clone(),
                            msg_input: "".to_string(),
                            users: vec![state.username.clone()],
                            messages: vec![],
                        });
                    }
                    _ => {}
                }
            }
            Screens::ChatScreen(state) => {
                match message {
                    Message::MsgInput(msg_input) => {
                        state.msg_input = msg_input;
                    }
                    Message::Send => {
                        state.messages.push(ChatMsg {
                            username: state.username.clone(),
                            msg: state.msg_input.clone(),
                        });
                        state.msg_input = "".to_string();
                    }
                    _ => {}
                }
            }
        }
    }

    fn view(&self) -> Element<Message> {
        match self {
            Screens::LoginScreen(state) => {
                column![
                    text_input("Username", &state.username)
                        .on_input(Message::Username)
                        .on_submit(Message::Login),
                    button("Login")
                        .on_press(Message::Login),
                ]
                    .padding(20)
                    .into()
            }
            Screens::ChatScreen(state) => {
                column![
                    text(format!("Welcome, {}!", state.username)),
                    row![
                        scrollable(
                            column(
                                state.messages
                                    .iter()
                                    .map(|msg| {
                                        text(format!("{} says {}", msg.username, msg.msg)).into()
                                    })
                                    .collect()
                            ),
                        )
                        .width(Length::FillPortion(3))
                        .height(Length::Fill)
                        .vertical_scroll(
                            Properties::new()
                                .width(10)
                                .scroller_width(10),
                        ),
                        scrollable(
                            column(
                                state.users
                                    .iter()
                                    .map(|user| {
                                        if user == &state.username {
                                            text(format!("{} (you)", user)).into()
                                        } else {
                                            text(user).into()
                                        }
                                    })
                                    .collect()
                            ),
                        )
                        .width(Length::FillPortion(1))
                        .vertical_scroll(
                            Properties::new()
                                .width(10)
                                .scroller_width(10),
                        ),
                    ]
                    .height(Length::Fill),
                    text_input("Send a message", &state.msg_input)
                        .on_input(Message::MsgInput)
                        .on_submit(Message::Send),
                    button("Send")
                        .on_press(Message::Send)
                ]
                    .padding(20)
                    .align_items(Alignment::Center)
                    .into()
            }
        }
    }
}
