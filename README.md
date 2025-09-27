# MW-TREE-ROUTER

breadboard + esp1 =  http://192.168.4.3/connections

breadboard + esp2 = http://192.168.4.2/connections

module + esp1 = http://192.168.4.1/connections

how to use:

1. cone project.
2. install pio
3. run for exemple: `.\upload.bat COM7 COM4`

```bash
.\upload.bat ... all ports (with esp32) to ulpoad the project
```

# Algorithmus‑Beschreibung

Dieser Abschnitt erklärt den inneren Ablauf des Tree Networking Protocol (TNP) ohne konkreten Code.

---

## 1. Adressabgleich

- Zwei Adressen (dynamische Listen von 16‑Bit‑Werten) werden Element für Element verglichen.
- Zunächst wird die Anzahl der hintereinander gleichen Elemente vom Listenanfang an gezählt („positive“ Treffer).
- Sobald ein Unterschied auftritt oder das Ende der kürzeren Liste erreicht ist, stoppt die Zählung.
- Die verbleibende Länge der getesteten Adresse (Restlänge) ergibt die „negative“ Größe.

---

## 2. Match‑Index

- Aus den Werten „positive“ und „negative“ wird ein einfacher Score berechnet:
  **Match‑Index = positive − negative**
- Ein hoher Index bedeutet:
  1. viele aneinanderhängende Übereinstimmungen am Anfang
  2. kurze Restlänge der Adresse
- Dieser Wert dient als Entscheidungsgrundlage für das Routing.

---

## 3. Gleichheitsprüfung

- Zwei Adressen gelten als identisch, wenn sie gleich lang sind und an jeder Position exakt denselben Wert haben.
- Wird eine exakte Übereinstimmung festgestellt, interpretiert der Knoten dies als „Ziel erreicht“.

---

## 4. Routing‑Logik

1. **Eingangskontrolle**

   - Liegen keine Verbindungen vor, wird abgebrochen und ein Fehlersignal („keine Verbindungen“) ausgegeben.

2. **Auswahl der besten Verbindung**

   - Für jede vorhandene Verbindung wird der Match‑Index gegen die gewünschte Zieladresse berechnet.
   - Es wird die Verbindung mit dem höchsten Match‑Index ausgewählt.

3. **Weiterleitung**

   - Die gewählte Verbindung liefert einen Pin‑Wert zurück und gibt zur Diagnose eine Meldung über die serielle Schnittstelle aus.

4. **Empfangsfall**

   - Wenn die Zieladresse mit der eigenen Adresse des Knotens übereinstimmt, wird nicht weitergeleitet, sondern die Verarbeitung endet mit dem Rückgabewert 0.

5. **Loadbalancing**
   - Wenn es zwei "gleich gute" (Matchindex equivalente indexe) gibt, wird die Lösung, mit der kleineren Adress-Länge ausgewählt.

---

## Zusammenfassung

- **Präfix‑Matching** bestimmt, wie gut eine Verbindung zur Zieladresse passt.
- **Match‑Index** gewichtet lange gemeinsame Präfixe und kurze Reste positiv.
- **Exakte Gleichheit** stoppt das Routing am Bestimmungsort.
- **Iterative Auswahl** stellt sicher, dass jeder Knoten stets den optimalen Pfad nutzt.

```

```
