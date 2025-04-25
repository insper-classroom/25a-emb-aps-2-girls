import sys
import glob
import serial
import pyautogui
import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
import time
# import threading
pyautogui.PAUSE = 0.0


def golpe_especial_mpu(axis, value):
    #TROVÃO NO PERSONAGEM ROXO
    if axis == 2:
        pyautogui.keyDown("left")
        time.sleep(0.05)
        pyautogui.keyUp("left")

        time.sleep(0.03)

        pyautogui.keyDown("left")
        time.sleep(0.05)
        pyautogui.keyUp("left")

        time.sleep(0.03)

        pyautogui.keyDown("s")
        time.sleep(0.05)
        pyautogui.keyUp("s")

estado_anterior = {
    "x": "middle",
    "y": "middle",  
}

def controle_teclas_setas(ser):
    teclas_btn = {
        0: "z",      # A
        1: "x",      # B
        2: "a",      # X
        3: "s",      # Y
        4: "LShift",  # SELECT
        5: "Enter",  # START
    }

    while True:
        sync_byte = ser.read(size=1)
        if not sync_byte:
            continue

        if sync_byte[0] == 0xFF:
            data = ser.read(size=3)
            if len(data) < 3:
                continue
            axis, value = parse_data(data)
            print(f"[JOYSTICK] Eixo: {axis} | Valor: {value}")
            if axis == 0: 
                if value == 3 and estado_anterior["x"] != "right":
                    pyautogui.keyUp("left")
                    pyautogui.keyDown("right")
                    estado_anterior["x"] = "right"
                elif value == 4 and estado_anterior["x"] != "left":
                    pyautogui.keyUp("right")
                    pyautogui.keyDown("left")
                    estado_anterior["x"] = "left"
                elif value == 0 and estado_anterior["x"] != "middle":
                    pyautogui.keyUp("left")
                    pyautogui.keyUp("right")
                    estado_anterior["x"] = "middle"

            elif axis == 1: 
                if value == 1 and estado_anterior["y"] != "up":
                    pyautogui.keyUp("down")
                    pyautogui.keyDown("up")
                    estado_anterior["y"] = "up"
                elif value == 2 and estado_anterior["y"] != "down":
                    pyautogui.keyUp("up")
                    pyautogui.keyDown("down")
                    estado_anterior["y"] = "down"
                elif value == 0 and estado_anterior["y"] != "middle":
                    pyautogui.keyUp("up")
                    pyautogui.keyUp("down")
                    estado_anterior["y"] = "middle"

        elif sync_byte[0] == 0xFE:
            data = ser.read(size=3)
            if len(data) < 3:
                continue
            btn_id = data[0]
            pressed = data[1]
            if pressed and btn_id in teclas_btn:
                tecla = teclas_btn[btn_id]
                # print(f"tecla = {tecla}")
                pyautogui.keyDown(teclas_btn[btn_id])
                time.sleep(0.2) 
                pyautogui.keyUp(teclas_btn[btn_id])

        elif sync_byte[0] == 0xFD:
            data = ser.read(size=3)
            if len(data) < 3:
                continue
            # print(data)
            axis, value = parse_data(data)
            golpe_especial_mpu(axis, value)

def serial_ports():
    """Retorna uma lista das portas seriais disponíveis na máquina."""
    ports = []
    if sys.platform.startswith('win'):
        # Windows
        for i in range(1, 256):
            port = f'COM{i}'
            try:
                s = serial.Serial(port)
                s.close()
                ports.append(port)
            except (OSError, serial.SerialException):
                pass
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # Linux/Cygwin
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        # macOS
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Plataforma não suportada para detecção de portas seriais.')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

def parse_data(data):
    """Interpreta os dados recebidos do buffer (axis + valor)."""
    axis = data[0]
    value = int.from_bytes(data[1:3], byteorder='little', signed=True)
    return axis, value

def conectar_porta( port_name, root, botao_conectar, status_label, mudar_cor_circulo):
    """Abre a conexão com a porta selecionada e inicia o loop de leitura."""
    if not port_name:
        messagebox.showwarning("Aviso", "Selecione uma porta serial antes de conectar.")
        return

    try:
        ser = serial.Serial(port_name, 115200, timeout=1)
        status_label.config(text=f"Conectado em {port_name}", foreground="green")
        mudar_cor_circulo("green")
        botao_conectar.config(text="Conectado")  # Update button text to indicate connection
        root.update()

        controle_teclas_setas(ser)

    except KeyboardInterrupt:
        print("Encerrando via KeyboardInterrupt.")
    except Exception as e:
        messagebox.showerror("Erro de Conexão", f"Não foi possível conectar em {port_name}.\nErro: {e}")
        mudar_cor_circulo("red")
    finally:
        ser.close()
        status_label.config(text="Conexão encerrada.", foreground="red")
        mudar_cor_circulo("red")



def criar_janela():
    root = tk.Tk()
    root.title("Controle de Mouse")
    root.geometry("400x250")
    root.resizable(False, False)

    # Dark mode color settings
    dark_bg = "#2e2e2e"
    dark_fg = "#ffffff"
    accent_color = "#007acc"
    root.configure(bg=dark_bg)

    style = ttk.Style(root)
    style.theme_use("clam")
    style.configure("TFrame", background=dark_bg)
    style.configure("TLabel", background=dark_bg, foreground=dark_fg, font=("Segoe UI", 11))
    style.configure("TButton", font=("Segoe UI", 10, "bold"),
                    foreground=dark_fg, background="#444444", borderwidth=0)
    style.map("TButton", background=[("active", "#555555")])
    style.configure("Accent.TButton", font=("Segoe UI", 12, "bold"),
                    foreground=dark_fg, background=accent_color, padding=6)
    style.map("Accent.TButton", background=[("active", "#005f9e")])

    # Updated combobox styling to match the dark GUI color
    style.configure("TCombobox",
                    fieldbackground=dark_bg,
                    background=dark_bg,
                    foreground=dark_fg,
                    padding=4)
    style.map("TCombobox", fieldbackground=[("readonly", dark_bg)])

    # Main content frame (upper portion)
    frame_principal = ttk.Frame(root, padding="20")
    frame_principal.pack(expand=True, fill="both")

    titulo_label = ttk.Label(frame_principal, text="Controle de Mouse", font=("Segoe UI", 14, "bold"))
    titulo_label.pack(pady=(0, 10))

    porta_var = tk.StringVar(value="")

    botao_conectar = ttk.Button(
        frame_principal,
        text="Conectar e Iniciar Leitura",
        style="Accent.TButton",
        command=lambda: conectar_porta(porta_var.get(), root, botao_conectar, status_label, mudar_cor_circulo)
    )
    botao_conectar.pack(pady=10)

    # Create footer frame with grid layout to host status label, port dropdown, and status circle
    footer_frame = tk.Frame(root, bg=dark_bg)
    footer_frame.pack(side="bottom", fill="x", padx=10, pady=(10, 0))

    # Left: Status label
    status_label = tk.Label(footer_frame, text="Aguardando seleção de porta...", font=("Segoe UI", 11),
                            bg=dark_bg, fg=dark_fg)
    status_label.grid(row=0, column=0, sticky="w")

    # Center: Port selection dropdown
    portas_disponiveis = serial_ports()
    if portas_disponiveis:
        porta_var.set(portas_disponiveis[0])
    port_dropdown = ttk.Combobox(footer_frame, textvariable=porta_var,
                                 values=portas_disponiveis, state="readonly", width=10)
    port_dropdown.grid(row=0, column=1, padx=10)

    # Right: Status circle (canvas)
    circle_canvas = tk.Canvas(footer_frame, width=20, height=20, highlightthickness=0, bg=dark_bg)
    circle_item = circle_canvas.create_oval(2, 2, 18, 18, fill="red", outline="")
    circle_canvas.grid(row=0, column=2, sticky="e")

    footer_frame.columnconfigure(1, weight=1)

    def mudar_cor_circulo(cor):
        circle_canvas.itemconfig(circle_item, fill=cor)

    root.mainloop()





if __name__ == "__main__":
    criar_janela()

