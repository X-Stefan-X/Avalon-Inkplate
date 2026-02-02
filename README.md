#Avalon-Inkplate 

Nutzt ein Inkplate 6 für die Anzeige wichtiger Navigationsdaten auf einer Segelyacht. 

Der dritte Versuch. Erster war ohne FreeRTOS gebuat. Zweiter war mit SensESP und jetzt direkt mit Websocket und FreeRTOS. So ist das Abrufen der Daten von SignalK getrennt von der Anzeige der Daten. Die Daten teilen sich auch in zwei Gruppen und werden unterschiedlich oft neu Dargestellt.


Features
	•	Unterstützung für Inkplate 6 Displays mit E-Paper-Technologie.
	•	WebSocket zu Signal K 
    •	Beim Start ein Foto vom Boot. ;)
    •	Daten"klassen" für unterschiedlichen Refresh
    •	Umrechnen der gelieferten Daten in Anzeigedaten (Nautisch EU)
    •	Verbindungsverlust Erkennung
    
