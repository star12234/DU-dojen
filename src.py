import pyttsx3
import comtypes
import comtypes.client
import win32gui
import pythoncom
import keyboard  # 키보드 입력 후킹

# UIAutomation DLL 로딩
UIA_dll = comtypes.client.GetModule('UIAutomationCore.dll')
uia = comtypes.client.CreateObject(UIA_dll.CUIAutomation._reg_clsid_, interface=UIA_dll.IUIAutomation)

class FocusChangeHandler(comtypes.COMObject):
    _com_interfaces_ = [UIA_dll.IUIAutomationFocusChangedEventHandler]

    def __init__(self, tts_engine):
        super().__init__()
        self.tts_engine = tts_engine
        self.last_spoken = ""

    def HandleFocusChangedEvent(self, sender):
        try:
            control_type = sender.CurrentLocalizedControlType
            name = sender.CurrentName
            text = f"{control_type}: {name}" if name else control_type
            if text and text != self.last_spoken:
                print(f"[포커스 변경] {text}")
                self.tts_engine.say(text)
                self.tts_engine.runAndWait()
                self.last_spoken = text
        except Exception as e:
            print(f"오류: {e}")

class KoreanScreenReader:
    def __init__(self):
        self.tts_engine = pyttsx3.init()
        self.tts_engine.setProperty('rate', 180)

        # 한국어 음성 설정
        self.set_korean_voice()

        self.focus_handler = FocusChangeHandler(self.tts_engine)

    def set_korean_voice(self):
        voices = self.tts_engine.getProperty('voices')
        for voice in voices:
            if "ko_" in voice.id or "korean" in voice.name.lower():
                self.tts_engine.setProperty('voice', voice.id)
                print(f"선택된 한국어 음성: {voice.name}")
                return
        print("한국어 음성을 찾지 못했습니다. 기본 음성을 사용합니다.")

    def start_keyboard_listener(self):
        # 사용자가 키를 누를 때마다 읽어줌
        def on_key(event):
            key_name = event.name
            if len(key_name) == 1:
                print(f"[키 입력] {key_name}")
                self.tts_engine.say(key_name)
                self.tts_engine.runAndWait()
            else:
                special_keys = {
                    'space': '스페이스',
                    'enter': '엔터',
                    'backspace': '백스페이스',
                    'tab': '탭',
                    'shift': '쉬프트',
                    'ctrl': '컨트롤',
                    'alt': '알트',
                    'delete': '딜리트',
                    'caps lock': '캡스락',
                    'esc': '이스케이프',
                    'up': '위 방향키',
                    'down': '아래 방향키',
                    'left': '왼쪽 방향키',
                    'right': '오른쪽 방향키'
                }
                korean_key = special_keys.get(key_name, key_name)
                print(f"[특수 키 입력] {korean_key}")
                self.tts_engine.say(korean_key)
                self.tts_engine.runAndWait()

        keyboard.on_press(on_key)

    def run(self):
        # 포커스 변경 이벤트 등록
        uia.AddFocusChangedEventHandler(None, self.focus_handler)
        self.start_keyboard_listener()

        print("스크린 리더가 작동 중입니다. 종료하려면 Ctrl+C를 누르세요.")

        try:
            while True:
                pythoncom.PumpWaitingMessages()
        except KeyboardInterrupt:
            print("스크린 리더 종료 중...")
            uia.RemoveFocusChangedEventHandler(self.focus_handler)

if __name__ == "__main__":
    reader = KoreanScreenReader()
    reader.run()

