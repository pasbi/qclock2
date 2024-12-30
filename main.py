#!/usr/bin/env python3

from PyQt5.QtWidgets import (
        QApplication,
        QWidget,
)

from PyQt5.QtGui import (
        QBrush,
        QColor,
        QPainter,
        QPen,
        QFont,
)

from PyQt5.QtCore import (
        QDateTime,
        QMargins,
        QPointF,
        QRectF,
        QSizeF,
        QTimer,
        Qt,
)

import sys
import numpy as np

column_count = 11
row_count = 10
letters = ( "ESKISTAFÜNF"
          + "ZEHNZWANZIG"
          + "DREIVIERTEL"
          + "VORFUNKNACH"
          + "HALBAELFÜNF"
          + "EINSXAMZWEI"
          + "DREIPMJVIER"
          + "SECHSNLACHT"
          + "SIEBENZWÖLF"
          + "ZEHNEUNKUHR")

word_coordinates = {
    # Always on
    "ES": (0, 0),
    "IST": (0, 3),

    # hours
    "zwölf": (8, 6),
    "eins": (5, 0),
    "zwei": (5, 7),
    "drei": (6, 0),
    "vier": (6, 7),
    "fünf": (4, 7),
    "sechs": (7, 0),
    "sieben": (8, 0),
    "acht": (7, 7),
    "neun": (9, 3),
    "zehn": (9, 0),
    "elf": (4, 5),

    # Minutes:
    "FÜNF": (0, 7),
    "ZEHN": (1, 0),
    "VIERTEL": (2, 4),
    "ZWANZIG": (1, 4),
    "DREIVIERTEL": (2, 0),
    "HALB": (4, 0),
    "UHR": (9, 8),

    # Vor/Nach
    "VOR": (3, 0),
    "NACH": (3, 7),
}

assert column_count * row_count == len(letters)

def store_painter(painter):
    class Storage:
        def __init__(self, painter):
            self.painter = painter
        
        def __enter__(self):
            painter.save()

        def __exit__(self, exc_type, exc_val, exc_tb):
            painter.restore()

    return Storage(painter)

def pen(**kwargs):
    pen = QPen()
    for k in ["Width", "Color"]:
        if k in kwargs:
            getattr(pen, "set" + k)(kwargs[k])
    return pen

def point(c):
    cx, cy = c
    return QPointF(cx, cy)

def draw_ellipse(painter, center, r):
    painter.drawEllipse(QRectF(point(center - r), QSizeF(r * 2.0, r * 2.0)))

class Clock(QWidget):
    margin = 50
    draw_debug_lines = False
    active_letters = set()
    corners = 0

    def __init__(self):
        super(Clock, self).__init__()

    def set_words(self, words, corners):
        self.active_letters = set()
        self.corners = corners
        for word in words:
            row, column = word_coordinates[word]
            for i, _ in enumerate(word):
                self.active_letters.add((column + i, row))
        self.update()

    def set_time(self, time):
        keys = list(word_coordinates.keys())
        words = [keys[0], keys[1]]
        h = time.hour()
        m = time.minute()

        print(f"{h:02}:{m:02}")

        if m >= 25:
            h = h + 1
        h %= 12

        words += {
                0: ["UHR"],
                5: ["FÜNF", "NACH"],
                10: ["ZEHN", "NACH"],
                15: ["VIERTEL", "NACH"],
                20: ["ZWANZIG", "NACH"],
                25: ["FÜNF", "VOR", "HALB"],
                30: ["HALB"],
                35: ["FÜNF", "NACH", "HALB"],
                40: ["ZWANZIG", "VOR"],
                45: ["VIERTEL", "VOR"],
                50: ["ZEHN", "VOR"],
                55: ["FÜNF", "VOR"],
        }[(m // 5) * 5]

        words.append(keys[h + 2])
        self.set_words(words, m % 5)

    def paintEvent(self, e):
        painter = QPainter(self)
        painter.fillRect(self.rect(), Qt.black)
        letters_rect = self.rect().marginsRemoved(QMargins(*[self.margin] * 4))
        cell_size = QSizeF(letters_rect.width() / column_count,
                           letters_rect.height() / row_count)

        if self.draw_debug_lines:
            with store_painter(painter):
                painter.setPen(pen(Width=4, Color=Qt.magenta))
                painter.drawRect(letters_rect)

        font = QFont()
        radius = 8
        font.setPixelSize(100)
        active_color = Qt.white
        inactive_color = QColor(0x20, 0x20, 0x20)
        
        for i, c in enumerate(letters):
            xi = i % column_count
            yi = i // column_count
            x = xi * cell_size.width() + self.margin
            y = yi * cell_size.height() + self.margin
            rect = QRectF(QPointF(x, y), cell_size)
            with store_painter(painter):
                if self.draw_debug_lines:
                    painter.setPen(pen(Width=1, Color=Qt.magenta))
                    painter.fillRect(rect, QColor(0xff, 0xff, 0, 0x20))
                    painter.drawRect(rect)
                    painter.drawText(rect, f"{i} [{xi}, {yi}]")
            with store_painter(painter):
                painter.setFont(font)
                if (xi, yi) in self.active_letters:
                    p = pen(Color=active_color)
                else:
                    p = pen(Color=inactive_color)
                painter.setPen(p)
                painter.drawText(rect, Qt.AlignCenter, f"{letters[i]}")

        corners = np.array([(0, 0), (self.width(), 0), (0, self.height()), (self.width(), self.height())])

        with store_painter(painter):
            painter.setPen(QPen())
            for i, c in enumerate(corners):
                p = (c == 0) * 2 - 1
                p = c + p * self.margin
                if self.draw_debug_lines:
                    painter.setPen(pen(Color=QColor(0x80, 0xff, 0x80)))
                    painter.drawLine(QPointF(p[0], p[1]), QPointF(c[0], c[1]))
                painter.setBrush(active_color if i < self.corners else inactive_color)
                t = 0.8
                draw_ellipse(painter, t * p + (1.0 - t) * c, radius)

        painter.end()

time = QDateTime.currentDateTime().time()

if __name__ == "__main__":
    app = QApplication(sys.argv)

    w = Clock()

    timer = QTimer()
    timer.setInterval(500)
    def on_timeout():
        global time
        #  time = time.addSecs(60)
        time = QDateTime.currentDateTime().time()
        w.set_time(time)

    timer.timeout.connect(on_timeout)
    timer.start()

    w.show()
    app.exec_()
