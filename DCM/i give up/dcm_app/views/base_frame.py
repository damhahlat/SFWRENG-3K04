import tkinter as tk
from tkinter import ttk

# Just to build the other windows off of
class BaseFrame(ttk.Frame):
    def __init__(self, parent, controller=None, **kwargs):
        super().__init__(parent, padding=10, **kwargs)
        self.controller = controller
