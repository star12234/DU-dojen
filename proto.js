// background.js

// 네이티브 메시징 호스트에 연결하는 함수
function connect() {
    var port = chrome.runtime.connectNative("com.example.screenreader");
    port.onMessage.addListener(function(response) {
      console.log("Received response:", response);
    });
    port.onDisconnect.addListener(function() {
      console.log("Disconnected from native host");
    });
    return port;
  }
  
  var port = connect();
  
  // 예시: 사용자가 특정 웹 페이지 이벤트(버튼 클릭 등)를 발생시켰을 때 메시지 전송
  function sendReadCommand(text) {
    var message = { command: "read", text: text };
    port.postMessage(message);
    console.log("Sent message:", message);
  }
  
  // 예시: 확장 프로그램이 로드될 때 테스트 메시지 전송
  sendReadCommand("안녕하세요, 웹 스크린 리더 통합 테스트입니다.");
  