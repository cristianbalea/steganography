// Steganografie. In poze se codifica text si apoi se decodifica aceeasi informatie si este afisata pe ecran.

#include "stdafx.h"
#include "common.h"
#include "OpenCVApplication.h"
#include <bitset>
#include <string>


using namespace std;

const int MAX_LENGTH = 1024;

void embedMessage(string message, int len) {
	/*
	* Pun la finalul stringului caracterul ~ pentru a-i marca sfarsitul
	* si a stii cand sa ma opresc din extragerea bitilor in momentul in 
	* care extrag mesajul.
	*/
	message.append("~");
	len++;
	char fname[MAX_PATH];

	if (openFileDlg(fname)) {
		/*
		* Declar imaginea sursa, si verific daca incape mesajul in aceasta. Conditia sa incapa e ca lungimea 
		* mesajului in binar sa fie mai mica sau egala cu numarul de pixeli * 3, pentru ca punem un bit pe
		* fiecare canal de culoare al fiecarui pixel.
		*/
		Mat src = imread(fname);

		int width = src.cols;
		int height = src.rows;

		if (width * height * 3 < len*8) {
			printf("Image size not enough for encoding!");
			return;
		}

		/*
		* Punem intr-un bitset octetii corespunzatori fiecarui caracter din mesajul pe care il incapsulam.
		*/
		bitset<8> binary[MAX_LENGTH];
		for (int i = 0; i < len; i++) {
			binary[i] = bitset<8>(message[i]);
		}

		/*
		* Din bitset, luam bit cu bit si il punem intr-un string.
		*/
		string someString;
		for (int i = 0; i < len; i++) {
			someString += (binary + i)->to_string();
		}

		/*
		* Declaram matricea destinatia, in care punem continutul imaginii alaturi de mesaj.
		*/
		Mat dst(height, width, CV_8UC3);

		/*
		* Inmultim lungimea cu 8, pentru ca am transformat fiecare caracter in 8 caractere de 0 si 1.
		* Numaram cu k de cate ori am pus un bit din mesaj, in imagine. Cand ajungem sa fie k >= len,
		* ne oprim din modificat canalele de culoare ale pixelilor.
		*/
		int k = 0;
		len *= 8;

		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				Vec3b originalChannels = src.at<Vec3b>(i, j);
				/*
				* Luam canalele de culoare din pixelii din imagine.
				*/
				uchar b = originalChannels[0];
				uchar g = originalChannels[1];
				uchar r = originalChannels[2];
				
				/*
				* Atat timp cat nu am ajuns la finalul mesajului, punem cate un bit pe fiecare canal de culoare
				* din fiecare pixel. Pentru a pune un bit de 0, facem o operatie AND cu 0x11111110 (0xfe),
				* iar pentru a pune un bit de 1, facem o operatie OR cu 0x00000001 (0x01).
				*/
				if (k < len) {
					(someString[k] == '0') ? b &= 0xfe : b |= 0x01;
					k++;
				}
				if (k < len) {
					(someString[k] == '0') ? g &= 0xfe : g |= 0x01;
					k++;
				}
				if (k < len) {
					(someString[k] == '0') ? r &= 0xfe : r |= 0x01;
					k++;
				}

				/*
				* Reconstruim Vec3b-ul cu canalele de culoare modificate(sau nu, daca s-a terminat mesajul)
				* si il punem in imagine destinatie la pixelul corespunzator.
				*/
				Vec3b modifiedChannels = Vec3b(b, g, r);
				dst.at<Vec3b>(i, j) = modifiedChannels;
			}
		}
		/*
		* Afisam imaginile iar apoi salvam imaginea cu mesajul, cu un nume specific.
		*/
		imshow("original image", src);
		imshow("modified image", dst);
		imwrite(strcat(fname, "_with_secret_message.bmp"), dst);
		waitKey();
	}
}

string binaryToString(string input) {
	/* Ne initializam un string in care vom pune rezultatul */
	string output;
	/* Calculam numarul de octeti, respectiv de caractere din input. */
	size_t num_bytes = input.length() / 8;
	for (size_t i = 0; i < num_bytes; i++) {
		/*
		* Extragem câte 8 caractere de 0 si 1, corespunzatoare unui octet
		* sau unui caracter ascii. Convertim cele 8 caractere de 0 si 1 la un
		* caracter ascii si il adaugam in output.
		*/
		string byte_str = input.substr(i * 8, 8);
		char byte = static_cast<char>(std::stoi(byte_str, nullptr, 2));
		output += byte;
	}
	/* Returnam sirul decodificat */
	return output;
}

boolean isTilda(char* str) {
	/* Verificam daca sirul str este egal cu sirul tilda, adica daca ultimul octet extras
	* din imagine reprezinta sfarsitul sirului.
	*/
	char tilda[] = "01111110";
	if (strcmp(str, tilda) == 0) {
		return true;
	}
	return false;
}

void extractMessage() {

	char fname[MAX_PATH];
	/*
	* Declar 2 stringuri, unul pentru mesajul in binar, unul pentru mesajul decodificat in ascii.
	*/
	string secretMessage;
	string binaryMessage;

	if (openFileDlg(fname)) {
		/* Deschid imaginea din care extrag mesajul, si initializez un string cu 00000000. 
		* In acesta, pastrez ultimii 8 biti extrasi, iar cand cei 8 biti corespund cu caracterul tilda
		* in binar, care marcheaza fisierul, ma opresc din extras bitii, variabila read devenind false.
		*/
		Mat src = imread(fname);

		int width = src.cols;
		int height = src.rows;


		char currentCharacter[] = "00000000";
		bool read = true;

		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				Vec3b originalChannels = src.at<Vec3b>(i, j);
				/* Extrag in variabilele uchar b, g, r canalele de culoare din pixelul curent. */

				uchar b = originalChannels[0];
				uchar g = originalChannels[1];
				uchar r = originalChannels[2];


				/* Atât timp cat nu am dat de tilda, care produce read = false,
				* extrag cate un bit din fiecare canal de culoare, si il pun atat in
				* mesaj cat si in currentCharacter.
				*/
				if (read) {
					uchar lastBit = b & 0x01;
					if ((int)lastBit) {
						binaryMessage += '1';
						strcpy(currentCharacter, currentCharacter + 1);
						strcat(currentCharacter, "1");
					}
					else {
						binaryMessage += '0';
						strcpy(currentCharacter, currentCharacter + 1);
						strcat(currentCharacter, "0");
					}
					if (isTilda(currentCharacter)) {
						read = false;
					}
				}
				if (read) {
					uchar lastBit = g & 0x01;
					if ((int)lastBit) {
						binaryMessage += '1';
						strcpy(currentCharacter, currentCharacter + 1);
						strcat(currentCharacter, "1");
					}
					else {
						binaryMessage += '0';
						strcpy(currentCharacter, currentCharacter + 1);
						strcat(currentCharacter, "0");
					}
					if (isTilda(currentCharacter)) {
						read = false;
					}
				}
				if (read) {
					uchar lastBit = r & 0x01;
					if ((int)lastBit) {
						binaryMessage += '1';
						strcpy(currentCharacter, currentCharacter + 1);
						strcat(currentCharacter, "1");
					}
					else {
						binaryMessage += '0';
						strcpy(currentCharacter, currentCharacter + 1);
						strcat(currentCharacter, "0");
					}
					if (isTilda(currentCharacter)) {
						read = false;
					}
				}
			}
		}

		/* Transform din binar in string, elimin tilda din mesaj si il afisez in consola. */
		secretMessage = binaryToString(binaryMessage);
		secretMessage.pop_back();
		cout << secretMessage;

		cout << endl;
		waitKey();
	}

}

void embedMessage_(string message, int len, char* filename) {
	char fname[MAX_PATH];

	/*
	* Analog cu functiile anterioare, transform stringul in binar.
	*/
	bitset<8> binary[MAX_LENGTH];
	for (int i = 0; i < len; i++) {
		binary[i] = bitset<8>(message[i]);
	}

	string someString;
	for (int i = 0; i < len; i++) {
		someString += (binary + i)->to_string();
	}

	int k = 0;
	len *= 8;

	/*
	* Vreau ca fiecare imagine sa aiba forma patratica, astfel ca voi calcula
	* cea mai mica dimensiune pentru un patrat astfel incat sa incapa mesajul
	*/
	float rad = sqrt(len);

	int height = ceil(rad);
	int width = height;

	/*
	* Declar imaginea destinatie, si o initializez cu alb.
	*/

	Mat dst(height * 3, width * 3, CV_8UC3);

	for (int i = 0; i < height * 3; i++) {
		for (int j = 0; j < width * 3; j++) {
			dst.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
		}
	}

	/*
	* Declar 2 puncte ce vor defini patratele ce vor fi desenate,
	* colturile stanga sus si dreapta jos.
	* X-ul punctelor va corespunde cu indicele i cu care se parcurge inaltimea, 
	* iar Y-ul punctelor va corespunde cu indicele j cu care parcurgem
	* latimea imaginii.
	*/
	Point topLeftCorner;
	Point bottomRightCorner;
	/*
	* Parcurg imaginea si desenez patrate de culori diferite pentru fiecare bit - albastru pentru 0,
	* roz pentru 1. Am folosit functia rectangle().
	*/

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {

			/*
			* Ne deplasam cu patratele pe linia i, fiecare coloana, iar apoi linia i + 1, fiecare coloana.
			*/
			topLeftCorner.x = i * 3;
			topLeftCorner.y = j * 3;
			bottomRightCorner.x = i * 3 + 2;
			bottomRightCorner.y = j * 3 + 2;

			/* Desenam patrate in numar egal cu lungimea sirului transformat in binar. in 
			* momentul in care s-a terminat sirul, ne oprim din desenat, iar daca nu acopera intreaga imagine,
			* vor ramane parti albe.
			*/
			if (k < len) {
				if (someString[k] == '0') {
					rectangle(dst, topLeftCorner, bottomRightCorner, Scalar(255, 255, 0), -1, LINE_8);
				}
				else {
					rectangle(dst, topLeftCorner, bottomRightCorner, Scalar(255, 0, 255), -1, LINE_8);
				}
				k++;
			}
			else {
				break;
			}
		}
	}

	/*
	* Construim calea imaginii cu numele primit din consola, afisam si salvam imaginea.
	*/
	char path[1024];
	strcpy(path, "C:\\Users\\balea\\Documents\\Facultate\\PI\\Proiect\\OpenCVApplication-VS2017_OCV340_basic\\Images\\");
	strcat(path, filename);
	strcat(path, ".bmp");

	imshow("modified image", dst);
	imwrite(path, dst);
	waitKey();
}

void extractMessage_() {
	/*
	* Am declarat variabile pentru mesajul in binar, si mesajul in ascii.
	* Ne alegem o imagine, si o parcurgem cu pas de 3, si verificam culoarea pixelului. 
	* Daca este albastru, atunci punem 0 in mesajul binar, daca e roz punem 1, iar daca este alb, sirul s-a terminat si iesim din parcurgere.
	*/
	char fname[MAX_PATH];

	string secretMessage;
	string binaryMessage;

	if (openFileDlg(fname)) {
		Mat src = imread(fname);

		int width = src.cols;
		int height = src.rows;

		int k = 0;

		for (int i = 0; i < height; i += 3) {
			for (int j = 0; j < width; j += 3) {
				/* Verificam daca pixelul e alb*/
				if (src.at<Vec3b>(j, i) == Vec3b(255, 255, 255)) {
					break;
				} else /* Altfel, in functie de culoare, punem 0 sau 1 in stringul binaryMessage */
				if (src.at<Vec3b>(j, i) == Vec3b(255, 0, 255)) {
					binaryMessage += '1';
				}
				else {
					binaryMessage += '0';
				}
			}
		}
		cout << endl;
		/*
		* Transformam mesajul din binar in string si apoi il afisam in consola.
		*/
		secretMessage = binaryToString(binaryMessage);
		cout << secretMessage;

		cout << endl;
		waitKey();
	}

}

int main()
{
	/*
	* In variabila s citesc mesajul, pe care-l pun in variabila string message. 
	* In variabila filename citesc numele pe care-l voi da fisierului produs 
	* in urma incapsularii mesajului cu metoda a doua.
	* In op, citesc continuu operatia ce se va face. La citirea lui 0, programul se incheie.
	*/
	char s[1024];
	char filename[1024];
	string message;

	int op;
	do
	{
		destroyAllWindows();
		/*
		* Optiunile 1 si 2 corespund primei metode de steganografie, iar optiunile 3 si 4 
		* celei de-a doua. Optiunea 0 reprezinta incheierea programului.
		* 
		*/
		printf("IP Project - Steganography:\n");
		printf(" 1 - Embed Message\n");
		printf(" 2 - Extract Message\n");
		printf(" 3 - Embed Message 2nd\n");
		printf(" 4 - Extract Message 2nd\n");
		printf(" 0 - Exit\n\n");
		printf("Option: ");
		scanf("%d", &op);
		getc(stdin);
		switch (op)
		{
		case 1:
			/*
			* Citesc string-ul, il pun in message, si apelez functia embedMessage
			* cu string ul si cu lungimea lui.
			*/
			printf("The secret message to embed: ");
			fflush(stdin);
			scanf("%[^\n]", s);
			message = s;
			embedMessage(message, strlen(s));
			break;
		case 2:
			extractMessage();
			break;
		case 3:
			/*
			* Citesc string-ul si numele fisierului, si le trimit ca parametri functiei 
			* alaturi de lungimea stringului.
			*/
			printf("The secret message to embed: ");
			fflush(stdin);
			scanf("%[^\n]", s);
			message = s;
			printf("The name of the file: ");
			getc(stdin);
			scanf("%[^\n]", filename);
			embedMessage_(message, strlen(s), filename);
			break;
		case 4:
			extractMessage_();
			break;
		}
		cout << endl << endl;
	} while (op != 0);
	return 0;
}