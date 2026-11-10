// ============================================================================
//  The 36 clubs of the UEFA Champions League 2025/26 league phase, with
//  seeding pots, approximate Elo ratings, stadiums and core squads.
//
//  Format (one record per line, '|' separated):
//    T|CODE|Club name|Country|Pot|Elo|Stadium
//    POS|Shirt|Rating|Player name        (POS = GK, DF, MF, FW)
//
//  Squads/numbers/ratings are a best-effort snapshot of early 2025/26 and are
//  approximate. To customise, choose "Export team database" in the Teams menu,
//  edit teams.txt and restart: if teams.txt exists it is loaded instead.
// ============================================================================
#include <fstream>
#include <sstream>
#include "Team.h"
#include "HashMap.h"

namespace {

const char* kTeamData = R"DATA(
T|PSG|Paris Saint-Germain|FRA|1|2030|Parc des Princes
GK|30|84|Lucas Chevalier
GK|39|79|Matvey Safonov
DF|2|87|Achraf Hakimi
DF|5|85|Marquinhos
DF|51|84|Willian Pacho
DF|25|86|Nuno Mendes
DF|21|80|Lucas Hernandez
DF|6|79|Illia Zabarnyi
MF|17|88|Vitinha
MF|87|86|Joao Neves
MF|8|83|Fabian Ruiz
MF|33|80|Warren Zaire-Emery
MF|19|81|Lee Kang-in
FW|10|90|Ousmane Dembele
FW|7|88|Khvicha Kvaratskhelia
FW|14|85|Desire Doue
FW|29|82|Bradley Barcola
FW|9|80|Goncalo Ramos
T|RMA|Real Madrid|ESP|1|1985|Santiago Bernabeu
GK|1|89|Thibaut Courtois
GK|13|78|Andriy Lunin
DF|2|83|Dani Carvajal
DF|3|84|Eder Militao
DF|24|82|Dean Huijsen
DF|22|84|Antonio Rudiger
DF|12|85|Trent Alexander-Arnold
DF|18|80|Alvaro Carreras
DF|23|79|Ferland Mendy
DF|17|77|Raul Asencio
MF|5|89|Jude Bellingham
MF|8|88|Federico Valverde
MF|14|85|Aurelien Tchouameni
MF|6|83|Eduardo Camavinga
MF|15|84|Arda Guler
MF|19|78|Dani Ceballos
MF|30|77|Franco Mastantuono
FW|10|92|Kylian Mbappe
FW|7|90|Vinicius Junior
FW|11|84|Rodrygo
FW|21|80|Brahim Diaz
FW|16|75|Gonzalo Garcia
T|MCI|Manchester City|ENG|1|1960|Etihad Stadium
GK|25|87|Gianluigi Donnarumma
GK|1|79|James Trafford
DF|3|86|Ruben Dias
DF|24|85|Josko Gvardiol
DF|5|81|John Stones
DF|82|79|Rico Lewis
DF|27|79|Matheus Nunes
DF|6|80|Nathan Ake
DF|45|77|Abdukodir Khusanov
DF|21|81|Rayan Ait-Nouri
MF|16|89|Rodri
MF|20|84|Bernardo Silva
MF|4|84|Tijjani Reijnders
MF|47|85|Phil Foden
MF|14|79|Nico Gonzalez
MF|10|82|Rayan Cherki
MF|8|81|Mateo Kovacic
FW|9|91|Erling Haaland
FW|7|82|Omar Marmoush
FW|11|82|Jeremy Doku
FW|26|80|Savinho
FW|52|75|Oscar Bobb
T|BAY|Bayern Munich|GER|1|2005|Allianz Arena
GK|1|86|Manuel Neuer
GK|40|76|Jonas Urbig
DF|2|84|Dayot Upamecano
DF|4|84|Jonathan Tah
DF|3|82|Kim Min-jae
DF|19|83|Alphonso Davies
DF|27|81|Konrad Laimer
DF|44|79|Josip Stanisic
DF|21|78|Hiroki Ito
DF|23|75|Sacha Boey
MF|6|88|Joshua Kimmich
MF|45|83|Aleksandar Pavlovic
MF|8|80|Leon Goretzka
MF|10|89|Jamal Musiala
MF|20|75|Tom Bischof
MF|42|74|Lennart Karl
FW|9|91|Harry Kane
FW|17|87|Michael Olise
FW|14|86|Luis Diaz
FW|7|81|Serge Gnabry
FW|11|80|Nicolas Jackson
T|LIV|Liverpool|ENG|1|1980|Anfield
GK|1|89|Alisson Becker
GK|25|81|Giorgi Mamardashvili
DF|4|88|Virgil van Dijk
DF|5|83|Ibrahima Konate
DF|6|80|Milos Kerkez
DF|26|80|Andrew Robertson
DF|30|81|Jeremie Frimpong
DF|12|78|Conor Bradley
DF|2|78|Joe Gomez
MF|10|86|Alexis Mac Allister
MF|8|85|Dominik Szoboszlai
MF|38|85|Ryan Gravenberch
MF|7|87|Florian Wirtz
MF|17|80|Curtis Jones
MF|3|76|Wataru Endo
FW|11|89|Mohamed Salah
FW|9|87|Alexander Isak
FW|22|83|Hugo Ekitike
FW|18|83|Cody Gakpo
FW|14|77|Federico Chiesa
T|INT|Inter|ITA|1|1945|San Siro
GK|1|84|Yann Sommer
GK|13|77|Josep Martinez
DF|95|86|Alessandro Bastoni
DF|15|79|Francesco Acerbi
DF|6|80|Stefan de Vrij
DF|31|79|Yann Bisseck
DF|25|83|Manuel Akanji
DF|2|83|Denzel Dumfries
DF|32|84|Federico Dimarco
DF|30|78|Carlos Augusto
MF|23|87|Nicolo Barella
MF|20|85|Hakan Calhanoglu
MF|22|80|Henrikh Mkhitaryan
MF|8|77|Petar Sucic
MF|16|80|Davide Frattesi
MF|7|80|Piotr Zielinski
FW|10|88|Lautaro Martinez
FW|9|85|Marcus Thuram
FW|14|77|Ange-Yoan Bonny
FW|94|74|Francesco Pio Esposito
T|CHE|Chelsea|ENG|1|1930|Stamford Bridge
GK|1|80|Robert Sanchez
GK|12|76|Filip Jorgensen
DF|24|83|Reece James
DF|6|82|Levi Colwill
DF|29|80|Wesley Fofana
DF|23|80|Trevoh Chalobah
DF|3|83|Marc Cucurella
DF|27|80|Malo Gusto
DF|4|78|Tosin Adarabioyo
MF|8|85|Enzo Fernandez
MF|25|87|Moises Caicedo
MF|10|87|Cole Palmer
MF|17|77|Andrey Santos
MF|45|78|Romeo Lavia
FW|20|83|Joao Pedro
FW|9|78|Liam Delap
FW|7|82|Pedro Neto
FW|41|80|Estevao
FW|11|78|Jamie Gittens
FW|49|79|Alejandro Garnacho
T|BVB|Borussia Dortmund|GER|1|1840|Signal Iduna Park
GK|1|85|Gregor Kobel
GK|33|72|Alexander Meyer
DF|4|83|Nico Schlotterbeck
DF|25|78|Niklas Sule
DF|3|79|Waldemar Anton
DF|5|77|Ramy Bensebaini
DF|26|78|Julian Ryerson
DF|24|76|Daniel Svensson
DF|2|77|Yan Couto
MF|20|80|Marcel Sabitzer
MF|8|80|Felix Nmecha
MF|13|79|Pascal Gross
MF|7|78|Jobe Bellingham
MF|10|81|Julian Brandt
MF|17|75|Carney Chukwuemeka
FW|9|85|Serhou Guirassy
FW|27|80|Karim Adeyemi
FW|14|78|Maximilian Beier
FW|21|75|Fabio Silva
T|BAR|FC Barcelona|ESP|1|1975|Spotify Camp Nou
GK|13|84|Joan Garcia
GK|25|79|Wojciech Szczesny
DF|23|85|Jules Kounde
DF|5|84|Pau Cubarsi
DF|4|82|Ronald Araujo
DF|3|82|Alejandro Balde
DF|24|79|Eric Garcia
DF|15|78|Andreas Christensen
DF|18|75|Gerard Martin
MF|8|89|Pedri
MF|21|85|Frenkie de Jong
MF|6|82|Gavi
MF|20|83|Dani Olmo
MF|16|82|Fermin Lopez
MF|17|77|Marc Casado
MF|22|74|Marc Bernal
FW|10|90|Lamine Yamal
FW|11|88|Raphinha
FW|9|85|Robert Lewandowski
FW|7|81|Ferran Torres
FW|14|81|Marcus Rashford
T|ARS|Arsenal|ENG|2|2015|Emirates Stadium
GK|1|86|David Raya
GK|13|78|Kepa Arrizabalaga
DF|2|87|William Saliba
DF|6|87|Gabriel Magalhaes
DF|12|83|Jurrien Timber
DF|33|81|Riccardo Calafiori
DF|4|80|Ben White
DF|5|81|Piero Hincapie
DF|3|76|Cristhian Mosquera
DF|49|77|Myles Lewis-Skelly
MF|41|88|Declan Rice
MF|8|86|Martin Odegaard
MF|36|85|Martin Zubimendi
MF|23|82|Mikel Merino
MF|10|84|Eberechi Eze
MF|16|78|Christian Norgaard
FW|7|88|Bukayo Saka
FW|14|84|Viktor Gyokeres
FW|11|81|Gabriel Martinelli
FW|19|81|Leandro Trossard
FW|20|80|Noni Madueke
FW|29|82|Kai Havertz
T|LEV|Bayer Leverkusen|GER|2|1820|BayArena
GK|1|80|Mark Flekken
GK|28|73|Janis Blaswich
DF|12|82|Edmond Tapsoba
DF|4|78|Jarell Quansah
DF|5|79|Loic Bade
DF|20|84|Alejandro Grimaldo
DF|21|77|Lucas Vazquez
DF|13|74|Arthur
DF|44|74|Jeanuel Belocian
MF|8|80|Robert Andrich
MF|25|80|Exequiel Palacios
MF|24|80|Aleix Garcia
MF|10|79|Malik Tillman
MF|6|76|Ezequiel Fernandez
MF|30|77|Ibrahim Maza
FW|14|81|Patrik Schick
FW|35|73|Christian Kofane
FW|19|76|Ernest Poku
FW|7|77|Eliesse Ben Seghir
T|ATM|Atletico Madrid|ESP|2|1870|Riyadh Air Metropolitano
GK|13|86|Jan Oblak
GK|1|77|Juan Musso
DF|2|81|Jose Maria Gimenez
DF|24|81|Robin Le Normand
DF|15|78|Clement Lenglet
DF|17|81|David Hancko
DF|18|77|Marc Pubill
DF|16|79|Nahuel Molina
DF|3|77|Matteo Ruggeri
MF|6|80|Koke
MF|8|81|Pablo Barrios
MF|4|80|Conor Gallagher
MF|5|78|Johnny Cardoso
MF|10|83|Alex Baena
MF|14|81|Marcos Llorente
MF|11|80|Thiago Almada
MF|23|79|Nicolas Gonzalez
FW|19|88|Julian Alvarez
FW|7|84|Antoine Griezmann
FW|9|81|Alexander Sorloth
FW|20|79|Giuliano Simeone
T|BEN|SL Benfica|POR|2|1790|Estadio da Luz
GK|1|82|Anatoliy Trubin
GK|24|70|Samuel Soares
DF|4|80|Antonio Silva
DF|30|79|Nicolas Otamendi
DF|44|79|Tomas Araujo
DF|17|78|Amar Dedic
DF|26|76|Samuel Dahl
DF|6|75|Alexander Bah
MF|8|80|Fredrik Aursnes
MF|5|77|Enzo Barrenechea
MF|20|79|Richard Rios
MF|18|77|Leandro Barreiro
MF|10|80|Georgiy Sudakov
FW|11|78|Dodi Lukebakio
FW|14|82|Vangelis Pavlidis
FW|9|75|Franjo Ivanovic
FW|21|76|Andreas Schjelderup
FW|25|74|Gianluca Prestianni
T|ATA|Atalanta|ITA|2|1810|Gewiss Stadium
GK|29|82|Marco Carnesecchi
GK|57|72|Marco Sportiello
DF|4|80|Isak Hien
DF|19|78|Berat Djimsiti
DF|23|76|Sead Kolasinac
DF|3|78|Odilon Kossounou
DF|77|77|Davide Zappacosta
DF|16|77|Raoul Bellanova
DF|69|74|Honest Ahanor
MF|13|82|Ederson
MF|15|80|Marten de Roon
MF|8|79|Mario Pasalic
MF|6|77|Yunus Musah
MF|24|78|Lazar Samardzic
MF|17|83|Charles De Ketelaere
FW|11|84|Ademola Lookman
FW|9|80|Gianluca Scamacca
FW|90|79|Nikola Krstovic
FW|7|76|Kamaldeen Sulemana
T|VIL|Villarreal|ESP|2|1780|Estadio de la Ceramica
GK|1|79|Luiz Junior
GK|13|75|Diego Conde
DF|8|79|Juan Foyth
DF|12|78|Renato Veiga
DF|4|78|Rafa Marin
DF|2|77|Logan Costa
DF|23|77|Sergi Cardona
DF|15|76|Santiago Mourino
DF|24|77|Alfonso Pedraza
MF|10|80|Dani Parejo
MF|14|78|Santi Comesana
MF|18|78|Pape Gueye
MF|16|80|Thomas Partey
MF|20|79|Alberto Moleiro
MF|17|77|Tajon Buchanan
FW|7|80|Gerard Moreno
FW|22|80|Ayoze Perez
FW|9|79|Georges Mikautadze
FW|19|79|Nicolas Pepe
FW|21|74|Tani Oluwaseyi
T|JUV|Juventus|ITA|2|1820|Allianz Stadium
GK|16|81|Michele Di Gregorio
GK|1|77|Mattia Perin
DF|3|84|Gleison Bremer
DF|15|79|Pierre Kalulu
DF|4|78|Federico Gatti
DF|6|77|Lloyd Kelly
DF|27|80|Andrea Cambiaso
DF|32|75|Juan Cabal
DF|2|74|Joao Mario
MF|5|81|Manuel Locatelli
MF|19|81|Khephren Thuram
MF|8|80|Teun Koopmeiners
MF|22|79|Weston McKennie
MF|10|84|Kenan Yildiz
FW|9|82|Dusan Vlahovic
FW|30|82|Jonathan David
FW|20|80|Lois Openda
FW|7|80|Francisco Conceicao
FW|11|78|Edon Zhegrova
T|SGE|Eintracht Frankfurt|GER|2|1750|Deutsche Bank Park
GK|40|77|Kaua Santos
GK|23|75|Michael Zetterer
DF|4|80|Robin Koch
DF|3|78|Arthur Theate
DF|33|75|Nnamdi Collins
DF|13|77|Rasmus Kristensen
DF|21|78|Nathaniel Brown
DF|5|74|Aurele Amenda
MF|27|77|Mario Gotze
MF|15|78|Ellyes Skhiri
MF|16|79|Hugo Larsson
MF|42|78|Can Uzun
MF|8|77|Fares Chaibi
MF|36|76|Ansgar Knauff
FW|9|79|Jonathan Burkardt
FW|20|79|Ritsu Doan
FW|7|75|Elye Wahi
FW|19|74|Jean-Matteo Bahoya
FW|18|74|Michy Batshuayi
T|CLB|Club Brugge|BEL|2|1740|Jan Breydel Stadium
GK|22|78|Simon Mignolet
GK|29|73|Nordin Jackers
DF|44|76|Brandon Mechele
DF|4|77|Joel Ordonez
DF|64|75|Kyriani Sabbe
DF|14|75|Bjorn Meijer
DF|65|75|Joaquin Seys
DF|41|75|Hugo Siquet
DF|5|74|Zaid Romero
MF|20|79|Hans Vanaken
MF|15|78|Raphael Onyedika
MF|25|75|Aleksandar Stankovic
MF|10|76|Hugo Vetlesen
MF|27|74|Casper Nielsen
FW|8|80|Christos Tzolis
FW|9|75|Nicolo Tresoldi
FW|21|77|Carlos Forbs
FW|17|74|Romeo Vermant
FW|11|74|Mamadou Diakhon
T|TOT|Tottenham Hotspur|ENG|3|1800|Tottenham Hotspur Stadium
GK|1|82|Guglielmo Vicario
GK|31|74|Antonin Kinsky
DF|17|84|Cristian Romero
DF|37|83|Micky van de Ven
DF|23|82|Pedro Porro
DF|13|79|Destiny Udogie
DF|24|77|Djed Spence
DF|4|77|Kevin Danso
DF|3|76|Radu Dragusin
MF|6|80|Joao Palhinha
MF|30|80|Rodrigo Bentancur
MF|29|79|Pape Matar Sarr
MF|15|78|Lucas Bergvall
MF|7|83|Xavi Simons
MF|10|82|James Maddison
MF|8|77|Yves Bissouma
FW|19|80|Dominic Solanke
FW|9|78|Richarlison
FW|20|82|Mohammed Kudus
FW|22|79|Brennan Johnson
FW|28|76|Wilson Odobert
FW|11|77|Mathys Tel
T|PSV|PSV Eindhoven|NED|3|1770|Philips Stadion
GK|1|77|Matej Kovar
GK|16|72|Joel Drommel
DF|6|77|Ryan Flamingo
DF|4|75|Armando Obispo
DF|17|77|Mauro Junior
DF|18|75|Anass Salah-Eddine
DF|8|78|Sergino Dest
DF|3|74|Yarek Gasiorowski
MF|22|79|Jerdy Schouten
MF|23|79|Joey Veerman
MF|34|79|Ismael Saibari
MF|20|78|Guus Til
MF|10|77|Paul Wanner
MF|27|76|Dennis Man
FW|5|78|Ivan Perisic
FW|9|77|Ricardo Pepi
FW|11|75|Couhaib Driouech
FW|25|74|Myron Boadu
T|AJA|Ajax|NED|3|1720|Johan Cruijff ArenA
GK|1|76|Vitezslav Jaros
GK|22|73|Remko Pasveer
DF|37|77|Josip Sutalo
DF|15|77|Youri Baas
DF|2|74|Lucas Rosa
DF|3|74|Anton Gaaei
DF|5|73|Owen Wijndal
DF|4|72|Aaron Bouwman
MF|8|79|Kenneth Taylor
MF|18|75|Davy Klaassen
MF|6|74|Youri Regeer
MF|16|75|Branco van den Boomen
MF|10|77|Oscar Gloukh
FW|25|75|Wout Weghorst
FW|11|77|Mika Godts
FW|23|76|Steven Berghuis
FW|9|77|Kasper Dolberg
FW|7|73|Oliver Edvardsen
T|NAP|Napoli|ITA|3|1860|Stadio Diego Armando Maradona
GK|1|81|Alex Meret
GK|32|78|Vanja Milinkovic-Savic
DF|22|81|Giovanni Di Lorenzo
DF|13|80|Amir Rrahmani
DF|4|81|Alessandro Buongiorno
DF|31|79|Sam Beukema
DF|17|78|Mathias Olivera
DF|37|76|Leonardo Spinazzola
DF|3|78|Miguel Gutierrez
MF|11|86|Kevin De Bruyne
MF|8|85|Scott McTominay
MF|68|83|Stanislav Lobotka
MF|99|81|Andre-Frank Zambo Anguissa
MF|6|78|Billy Gilmour
FW|9|81|Romelu Lukaku
FW|19|80|Rasmus Hojlund
FW|7|80|David Neres
FW|21|78|Matteo Politano
FW|70|78|Noa Lang
T|SCP|Sporting CP|POR|3|1800|Estadio Jose Alvalade
GK|1|80|Rui Silva
GK|12|72|Joao Virginia
DF|26|81|Ousmane Diomande
DF|25|81|Goncalo Inacio
DF|6|78|Zeno Debast
DF|22|75|Ivan Fresneda
DF|20|78|Maxi Araujo
DF|72|77|Eduardo Quaresma
MF|42|81|Morten Hjulmand
MF|5|78|Hidemasa Morita
MF|8|82|Pedro Goncalves
MF|17|82|Francisco Trincao
MF|23|77|Daniel Braganca
MF|7|79|Geovany Quenda
FW|97|80|Luis Suarez
FW|89|76|Fotis Ioannidis
FW|10|77|Geny Catamo
FW|19|72|Rodrigo Ribeiro
T|OLY|Olympiacos|GRE|3|1700|Karaiskakis Stadium
GK|88|78|Konstantinos Tzolakis
GK|1|72|Alexandros Paschalakis
DF|45|75|Panagiotis Retsos
DF|5|75|Lorenzo Pirola
DF|16|75|David Carmo
DF|3|74|Francisco Ortega
DF|23|75|Rodinei
DF|20|75|Costinha
MF|32|76|Santiago Hezze
MF|14|75|Dani Garcia
MF|22|76|Chiquinho
MF|10|78|Gelson Martins
MF|56|78|Daniel Podence
FW|9|80|Ayoub El Kaabi
FW|99|79|Mehdi Taremi
FW|27|75|Gabriel Strefezza
T|SLA|Slavia Praha|CZE|3|1660|Fortuna Arena
GK|36|77|Jindrich Stanek
GK|35|70|Jakub Markovic
DF|4|75|David Zima
DF|3|75|Tomas Holes
DF|5|74|Igoh Ogbu
DF|24|73|Stepan Chaloupek
DF|21|75|David Doudera
DF|18|73|Jan Boril
MF|17|77|Lukas Provod
MF|10|77|Christos Zafeiris
MF|19|75|Oscar Dorley
MF|23|74|Michal Sadilek
MF|26|75|Ivan Schranz
FW|13|74|Mojmir Chytil
FW|25|75|Tomas Chory
FW|11|73|Vasil Kusej
T|BOD|Bodo/Glimt|NOR|3|1690|Aspmyra Stadion
GK|12|77|Nikita Haikin
GK|1|68|Julian Lund
DF|15|74|Fredrik Bjorkan
DF|4|73|Odin Bjortuft
DF|6|72|Jostein Gundersen
DF|20|72|Fredrik Sjovold
DF|5|72|Villads Nielsen
DF|3|71|Haitam Aleesami
MF|7|76|Patrick Berg
MF|14|74|Ulrik Saltnes
MF|26|74|Hakon Evjen
MF|19|73|Sondre Brunstad Fet
MF|8|73|Isak Dybvik Maatta
FW|10|78|Jens Petter Hauge
FW|9|76|Kasper Hogh
FW|11|73|Ole Didrik Blomberg
T|MAR|Olympique de Marseille|FRA|3|1780|Orange Velodrome
GK|1|81|Geronimo Rulli
GK|12|70|Jeffrey de Lange
DF|5|80|Leonardo Balerdi
DF|28|80|Benjamin Pavard
DF|21|79|Nayef Aguerd
DF|4|75|CJ Egan-Riley
DF|32|78|Facundo Medina
DF|62|75|Amir Murillo
DF|33|75|Emerson Palmieri
MF|23|81|Pierre-Emile Hojbjerg
MF|19|77|Geoffrey Kondogbia
MF|18|77|Arthur Vermeeren
MF|8|78|Matt O'Riley
MF|6|77|Angel Gomes
MF|26|73|Bilal Nadir
FW|10|83|Mason Greenwood
FW|97|80|Pierre-Emerick Aubameyang
FW|14|78|Igor Paixao
FW|9|80|Amine Gouiri
FW|22|77|Timothy Weah
T|FCK|FC Copenhagen|DEN|4|1660|Parken
GK|1|76|Dominik Kotarski
GK|41|68|Theo Sander
DF|2|74|Gabriel Pereira
DF|6|75|Pantelis Hatzidiakos
DF|15|74|Marcos Lopez
DF|23|74|Rodrigo Huescas
DF|24|72|Birger Meling
DF|3|72|Junnosuke Suzuki
MF|12|74|Lukas Lerager
MF|8|75|Magnus Mattsson
MF|36|72|William Clem
MF|7|74|Viktor Claesson
MF|30|73|Elias Achouri
FW|11|76|Jordan Larsson
FW|14|73|Andreas Cornelius
FW|18|75|Youssoufa Moukoko
FW|10|75|Mohamed Elyounoussi
T|ASM|AS Monaco|FRA|4|1730|Stade Louis II
GK|16|77|Philipp Kohn
GK|1|74|Radoslaw Majecki
DF|5|77|Thilo Kehrer
DF|22|77|Mohammed Salisu
DF|2|79|Vanderson
DF|12|77|Caio Henrique
DF|13|74|Christian Mawissa
DF|3|76|Eric Dier
MF|6|80|Denis Zakaria
MF|15|78|Lamine Camara
MF|17|79|Aleksandr Golovin
MF|11|80|Maghnes Akliouche
MF|8|77|Paul Pogba
MF|28|73|Mamadou Coulibaly
FW|18|78|Takumi Minamino
FW|9|79|Folarin Balogun
FW|31|76|Ansu Fati
FW|14|77|Mika Biereth
T|GAL|Galatasaray|TUR|4|1730|RAMS Park
GK|1|81|Ugurcan Cakir
GK|19|72|Gunay Guvenc
DF|6|79|Davinson Sanchez
DF|42|77|Abdulkerim Bardakci
DF|90|77|Wilfried Singo
DF|17|74|Eren Elmali
DF|23|75|Kaan Ayhan
DF|4|75|Ismail Jakobs
MF|34|79|Lucas Torreira
MF|8|79|Gabriel Sara
MF|20|80|Ilkay Gundogan
MF|11|76|Yunus Akgun
MF|7|77|Roland Sallai
FW|10|82|Leroy Sane
FW|45|86|Victor Osimhen
FW|9|79|Mauro Icardi
FW|53|78|Baris Alper Yilmaz
T|USG|Union Saint-Gilloise|BEL|4|1700|Lotto Park
GK|37|75|Kjell Scherpen
GK|1|68|Vic Chambaere
DF|16|75|Christian Burgess
DF|5|74|Kevin Mac Allister
DF|26|73|Ross Sykes
DF|25|75|Anan Khalaili
DF|48|73|Fedde Leysen
DF|22|72|Ousseynou Niang
MF|28|75|Charles Vanhoutte
MF|18|73|Kamiel Van de Perre
MF|7|74|Mohammed Fuseini
MF|10|74|Adem Zorgane
MF|8|72|Rob Schoofs
FW|12|76|Promise David
FW|13|74|Kevin Rodriguez
FW|11|74|Anouar Ait El Hadj
FW|30|73|Raul Florucz
T|QAR|Qarabag FK|AZE|4|1620|Tofiq Bahramov Stadium
GK|99|73|Mateusz Kochalski
GK|1|68|Shahrudin Mahammadaliyev
DF|13|72|Bahlul Mustafazada
DF|44|72|Elvin Cafarquliyev
DF|81|72|Kevin Medina
DF|55|71|Badavi Huseynov
DF|2|71|Matheus Silva
MF|8|73|Marko Jankovic
MF|6|72|Pedro Bicalho
MF|10|75|Abdellah Zoubir
MF|15|74|Leandro Andrade
MF|20|72|Toral Bayramov
FW|7|73|Juninho
FW|9|72|Camilo Duran
FW|19|72|Nariman Akhundzade
FW|17|70|Emmanuel Addai
T|ATH|Athletic Club|ESP|4|1780|San Mames
GK|1|83|Unai Simon
GK|27|74|Alex Padilla
DF|3|80|Dani Vivian
DF|4|77|Aitor Paredes
DF|18|74|Oscar de Marcos
DF|17|76|Yuri Berchiche
DF|2|75|Andoni Gorosabel
DF|12|75|Jesus Areso
DF|19|73|Adama Boiro
MF|23|76|Mikel Jauregizar
MF|24|77|Benat Prados
MF|8|80|Oihan Sancet
MF|16|76|Inigo Ruiz de Galarreta
MF|6|74|Mikel Vesga
MF|20|73|Unai Gomez
FW|10|85|Nico Williams
FW|9|80|Inaki Williams
FW|11|77|Gorka Guruzeta
FW|7|77|Alex Berenguer
FW|14|75|Robert Navarro
T|NEW|Newcastle United|ENG|4|1850|St James' Park
GK|1|82|Nick Pope
GK|32|78|Aaron Ramsdale
DF|4|80|Sven Botman
DF|5|79|Fabian Schar
DF|33|78|Dan Burn
DF|12|78|Malick Thiaw
DF|2|78|Kieran Trippier
DF|21|80|Tino Livramento
DF|3|79|Lewis Hall
MF|39|86|Bruno Guimaraes
MF|8|84|Sandro Tonali
MF|7|81|Joelinton
MF|41|77|Jacob Ramsey
MF|28|76|Joe Willock
FW|10|83|Anthony Gordon
FW|27|80|Nick Woltemade
FW|9|80|Yoane Wissa
FW|11|78|Harvey Barnes
FW|20|79|Anthony Elanga
FW|23|78|Jacob Murphy
T|PAF|Pafos FC|CYP|4|1570|Alphamega Stadium
GK|1|72|Jay Gorter
GK|93|70|Neofytos Michael
DF|4|74|David Luiz
DF|5|72|Derrick Luckassen
DF|16|72|David Goldar
DF|30|70|Bruno Langa
DF|22|70|Joao Correia
MF|8|73|Pepe
MF|10|74|Vlad Dragomir
MF|7|73|Domingos Quina
MF|34|72|Ivan Sunjic
FW|9|72|Anderson Silva
FW|11|73|Jaja
FW|17|74|Mislav Orsic
T|KAI|Kairat Almaty|KAZ|4|1500|Central Stadium Almaty
GK|1|70|Temirlan Anarbekov
GK|82|66|Sherkhan Kalmurza
DF|4|70|Aleksandr Martynovich
DF|25|69|Egor Sorokin
DF|2|69|Luis Mata
DF|24|68|Erkin Tapalov
DF|15|67|Damir Kassabulat
MF|6|71|Ofri Arad
MF|10|72|Jorginho
MF|7|70|Valeriy Gromyko
MF|8|69|Giorgi Zaria
MF|18|68|Adilet Sadybekov
FW|9|73|Dastan Satpaev
FW|11|70|Edmilson
FW|17|70|Ricardinho
)DATA";

// Splits "a|b|c" into fields.
ds::DynArray<std::string> splitPipe(const std::string& line) {
    ds::DynArray<std::string> out;
    std::string cur;
    for (char c : line) {
        if (c == '|') { out.push(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    out.push(cur);
    return out;
}

bool toInt(const std::string& s, int& out) {
    if (s.empty()) return false;
    int v = 0, i = 0;
    bool neg = false;
    if (s[0] == '-') { neg = true; i = 1; }
    for (; i < static_cast<int>(s.size()); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
        v = v * 10 + (s[i] - '0');
    }
    out = neg ? -v : v;
    return true;
}

bool parseDatabase(std::istream& in, ds::DynArray<Team>& teams, std::string& log) {
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty() || line[0] == '#') continue;
        ds::DynArray<std::string> f = splitPipe(line);
        if (f[0] == "T") {
            int pot = 0, elo = 0;
            if (f.size() != 7 || !toInt(f[4], pot) || !toInt(f[5], elo)) {
                log += "  [ERROR] line " + std::to_string(lineNo) + ": bad team record\n";
                return false;
            }
            Team t;
            t.id = teams.size();
            t.code = f[1]; t.name = f[2]; t.country = f[3];
            t.pot = pot; t.elo = elo; t.stadium = f[6];
            teams.push(t);
        } else {
            Position pos;
            int num = 0, rating = 0;
            if (teams.empty() || f.size() != 4 || !parsePosition(f[0], pos) ||
                !toInt(f[1], num) || !toInt(f[2], rating) || f[3].empty()) {
                log += "  [ERROR] line " + std::to_string(lineNo) + ": bad player record: " + line + "\n";
                return false;
            }
            Player p;
            p.pos = pos; p.number = num; p.rating = rating; p.name = f[3];
            teams.back().squad.push(p);
        }
    }
    return true;
}

}  // namespace

bool loadTeamDatabase(ds::DynArray<Team>& teams, std::string& log) {
    teams.clear();
    std::ifstream file("teams.txt");
    bool ok;
    if (file) {
        log += "Loading custom team database from teams.txt\n";
        ok = parseDatabase(file, teams, log);
    } else {
        std::istringstream in(kTeamData);
        ok = parseDatabase(in, teams, log);
    }
    if (!ok) return false;

    // League-wide validation: 36 teams, 9 per pot, unique codes
    if (teams.size() != 36) {
        log += "  [ERROR] the league phase needs exactly 36 teams (found " + std::to_string(teams.size()) + ")\n";
        return false;
    }
    int potCount[5] = {0, 0, 0, 0, 0};
    ds::HashMap<std::string, int> codes;
    for (Team& t : teams) {
        if (!validateAndCompleteSquad(t, log)) return false;
        if (codes.contains(t.code)) {
            log += "  [ERROR] duplicate team code " + t.code + "\n";
            return false;
        }
        codes.put(t.code, t.id);
        potCount[t.pot]++;
    }
    for (int p = 1; p <= 4; ++p) {
        if (potCount[p] != 9) {
            log += "  [ERROR] pot " + std::to_string(p) + " must contain 9 teams\n";
            return false;
        }
    }
    return true;
}

bool exportTeamDatabase(const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;
    out << "# UCL simulator team database. Edit and restart to use.\n"
           "# T|CODE|Club|Country|Pot|Elo|Stadium   then   POS|Shirt|Rating|Name\n";
    out << kTeamData;
    return static_cast<bool>(out);
}
