# Lidgren.Network C++ Port
C++ port of Lidgren.Network library for use in Beat Saber Multiplayer mod for Oculus Quest.
<br/>
Many features of the original library have not yet been implemented, either because they are not needed for Multiplayer mod, or because I am too lazy to implement them.

<br/>

List of important differences at the moment:
1. ReliableOrdered messages work like ReliableSequence
2. No support for fragmented messages
3. No support for strings longer than 127 characters
4. *Something else I probably forgot about*
