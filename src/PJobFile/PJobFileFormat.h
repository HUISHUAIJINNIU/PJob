#pragma once
#include "PJobFileError.h"
#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QVariant>

//! Kapselung des Zugriffs auf unser PJob-Dateiformat
/*!
  * @author Daniel Merget
  * @date 26.02.2010
  *
  * Bei der �bergabe von Pfaden folgendes beachten:
  *  - Strings sind grunds�tzlich Case-Sensitive
  *  - Pfade verwenden grunds�tzlich '/' als Trennzeichen
  *  - Pfade zu Ordnern enden immer mit '/'
  *  - Pfade beginnen niemals mit '/'
  */

class PJobFileFormat : public QFile
{
Q_OBJECT
public:

	//! Version des PJob Dateiformats. Muss erh�ht werden, wenn keine abw�rts Kompatibilit�t mehr gegeben ist.
	static const int c_version=2;
	//! Gr��e des Headers jeder Datei in einer PJob Datei ohne Dateiname.
	/*!
	 * Struktur: 1 Byte \n am Ende jedes Dateinamens, 8 Byte Modification Time, 4 Byte Dateigr��e, 4 Byte gepackte Dateigr��e
	 * ACHTUNG: Bei �nderungen der Struktur m�ssen alle Funktionen auf Korrektheit �berpr�ft werden und gegebenenfalls angepasst werden.
	 * Dies ist trotz der Variable c_fileHeaderSize n�tig, wenn Funktionen am Dateiheader(Auslesen/ Schreiben einzelner Attribute) arbeiten.
	 */
	static const int c_fileHeaderSize = 17;

        //! �ffnet eine .pjob-Datei. Falls diese nicht exisitert wird eine leere Datei (inklusive Header) erzeugt.
        /*!
         *  @param path kann relativ zum Arbeitsverzeichnis oder absolut angegeben werden.
         */
        PJobFileFormat(QString path);

        //! Destruktor verwirft alle ver�nderten Daten, wenn nicht voher flush() aufgerufen wurde.
        ~PJobFileFormat();

        //SCHREIBEN
        //! F�gt eine beliebige Datei sourceAbsolutePath (oder ein QByteArray) unter dem Ordner-/Dateipfad targetRelativePath hinzu. Gibt true zur�ck, falls erfolgreich. Wenn die Datei unter diesem Namen bereits existiert wird sie �berschrieben!
        /*!
         *  @param sourceAbsolutePath ist der Pfad der hinzuzuf�genden Datei. Darf auch relativ zum Pfad der .pjob-Datei angegeben werden.
         *  @param targetRelativePath gibt den Pfad an, welcher innerhalb der .pjob-Datei f�r die Datei reserviert werden soll. Falls NULL �bergeben wird, wird nur der Name der Datei ohne Ordnerpfad �bernommen.
         *  @param overwrite gibt an, ob Dateien, die bereits existieren �berschrieben werden sollen.
         */
        bool appendFile(QString sourceAbsolutePath, QString targetRelativePath = NULL, bool overwrite = true);

        bool appendFile(const QByteArray &source, QString targetRelativePath, quint64 mtime = 0, bool overwrite = true);
		
        //! F�gt ein QByteArray an die Datei an und erstellt die Map neu (muss einen g�ltigen Header enthalten)
        bool appendRaw(const QByteArray &source);

        //! Siehe appendFile(). Es werden nur einzelne Dateien �berschrieben, nicht der gesamte Ordner.
        bool appendFolder(QString sourceAbsolutePath, QString targetRelativePath = NULL, bool overwrite = true);

        //! Benennt eine Datei oder einen Ordner um.
        /*!
        *  ACHTUNG: �berpr�ft momentan in keiner Weise, ob das Umbenennen Sinn macht(z.B. bereits vorhandener Dateiname etc.)
        *
        *  Pr�ft erst ob ein Datei Pfad �bergeben wurde. Versucht andernfalls einen Order umzubennen.
        *  Gibt true zur�ck, wenn Dateien umbenannt wurden.
        */
        bool rename(QString oldPath, QString newPath);

        //! Speichert alle Ver�nderungen an der .pjob-Datei ab. Gibt true zur�ck, falls erfolgreich.
        void flush();

        //LESEN
        //! Gibt true zur�ck, falls sich eine Datei mit Namen relativePath im .pjob-File befindet
        bool contains(QString relativePath);

        //! Gibt true zur�ck, falls sich eine Datei im .pjob-File befindet, die mit relativePath beginnt
        bool containsDirectory(QString relativePath);

        //! liest eine Datei relativePath aus der .pjob-Datei aus und gibt diese als Bytearray zur�ck. Die Datei wird nicht gel�scht!
        QByteArray readFile(QString relativePath);

        //! liest eine Datei mit Namen relativePath inklusive zugeh�rigem Header ein und gibt sie als QBytearray zur�ck
        QByteArray readRaw(QString relativePath);

        //L�SCHEN
        //! entfernt eine Datei relativePath aus dem .pjob-File. Gibt true zur�ck, falls erfolgreich. Muss �ber flush() gespeichert werden
        bool removeFile(QString relativePath);

        //! L�scht eine Datei oder einen Ordner mit dem �bergebenen Pfad
        bool remove(QString relativePath);

        //ENTPACKEN
        //! entpackt eine oder mehrere Dateien aus dem .pjob-File in einen Ausgew�hlten Ordner
        /*!
         *  @param targetAbsolutePath gibt an, wohin die Dateien entpackt werden sollen. Falls NULL, wird Verzeichnis der .pjob-Datei entpackt.
         *  @param sourceRelativePath gibt an, welche Datei/welcher Ordner entpackt werden soll. Falls NULL, werden alle Dateien entpackt.
         *  @param overwrite gibt an, ob die Dateien beim Entpacken �berschrieben werden sollen.
         */
        void extract(QString targetAbsolutePath = NULL, QString sourceRelativePath = NULL, bool overwrite = true);

        //INFO
        //! Gibt eine QStringList zur�ck, welche alle Dateinamen (relativer Pfad) im .pjob-File enth�lt
        QStringList content();

        //! Gibt eine detaillierte Liste mit den Dateien in der .pjob-File und deren Eigenschaften zur�ck.
        /*!
        *  R�ckgabeformat ist f�r jede Datei eine Liste aus QVariants mit 3 Elementen. Die Elemente entsprechen
        *  in der folgenden Reihenfolge: fileName(QString), modificationTime(quint64), fileSize(int)
        */
        QList<QList<QVariant> > detailedContents(QString folder = "");

        //!Gibt die aktuelle Version zur�ck
        static int version();

signals:
    void output(QString fileName);
    void fileAdded(QList<QVariant> fileProperties);

private:
        struct intPair
        {
            int position, size;
        };

        //Hilfsfunktionen f�r Konstruktor und flush()
        bool isValid(); //**
        void createNewFile();
        void writeHeaderInformation(bool overwriteOldHeader);
        void map();

        //Hilfsfunktion f�r einfacheren Zugriff auf m_map
        void addToMap(QString relativePath, int position, int size);

        //Hilfsfunktion zum Anpassen von Pfadeingaben (wird nicht benutzt/ben�tigt)
        static QString adjustPath(QString path);

        //Hilfsfunktion zum �berpr�fen, ob beim Hinzuf�gen einer Datei Kollisionen mit dem Windows-Dateisystem entstehen k�nnten
        bool proofUnique(QString relativePath);

        //Integer lesen und schreiben
        static void writeInt32(quint32 input, QByteArray &array, int pos);
        static void writeInt64(quint64 input, QByteArray &array, int pos);
        static quint32 readInt32(const QByteArray &array, int pos);
        static quint64 readInt64(const QByteArray &array, int pos);

        //Hilfsfunktion zum Potenzieren
        static quint32 power(const int base, int exponent);

        //Member
        QString m_path;
        uint m_version;
        bool m_modified;

        QByteArray m_data;
        QMap<QString,intPair> m_map;
};
