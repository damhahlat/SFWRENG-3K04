import tkinter as tk

from dcm_app.controllers.app_controller import AppController


# This is the main loop
def main() -> None:
    root = tk.Tk()
    root.title("Pacemaker DCM")
    root.geometry("900x600")

    # Create the controller, which manages all screens
    app = AppController(root)
    app.show_welcome()

    root.mainloop()


if __name__ == "__main__":
    main()
