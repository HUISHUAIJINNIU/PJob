#pragma once
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariant>

//! Einfache Item basierte Baum Datenstruktur f�r PJobDirModel
/*!
 *  Jede PJobDirTree Instanz ist ein Item mit einer Liste von Kindern m_children
 *  und einem Satz von Daten, m_data. Jedes Item kann die Wurzel eines neuen Baums sein.
 *
 *	Daten werden in einer Liste von QVariants gespeichert um variable Datentypen zu erlauben.
 *  Items werden intern(findItem() etc.) anhand ihres ersten Dateneintrags identifiziert, der
 *  als String angenommen wird. Duplikate sind nicht erlaubt. Wird ein Kind eingef�gt, das ein
 *  Geschwister mit dem selben Namen hat(m_data[0]), wird dieses �berschrieben.
 *
 *  PJob Datei Verzeichnisstruktur:
 *  Die eigentlichen Funktionen zum erstellen einer Verzeichnisstruktur befinden sich am Ende
 *  der Klasse. Allgemein handelt es sich bei PJobDirTree um eine einfache Baum Struktur.
 *
 *  Jedes Item hat als ersten Wert den vollen Pfad, um einen einfachen Zugriff auf die PJob Datei
 *  zu erlauben. m_data besteht f�r Dateien in folgender Reihenfolge aus:
 *		fileName, modificationTime, fileSize.
 *  Verzeichnisse haben lediglich den ersten Dateneintrag(m_data.size() -> 1).
 *  Alle Daten werden beim einf�gen in f�r die GUI vormatierte Strings umgewandelt, siehe
 *  die statische Funktion formatDataToString(const QList<QVariant>& fileProperties).
 */

class PJobDirTree
{
public:
	PJobDirTree(PJobDirTree* parent = NULL, QList<QVariant> data = QList<QVariant>());
	~PJobDirTree(void);

	//! Erweitert das momentane Item um ein Item mit den Daten data
	/*!
	 *  Existiert bereits ein Kind mit selbem data[0], so wird dieses �berschrieben.
	 */
	PJobDirTree* append(QList<QVariant> data = QList<QVariant>());
	void deleteChild(PJobDirTree* item);
	QVariant data(int index);
	void setData(int index, QVariant value);

	PJobDirTree* parent();
	PJobDirTree* childAt(int row);
	int childCount();
	int columnCount();
	int row();

	PJobDirTree* findItem(QVariant value);

	//
	// Funktionen die speziell f�r die PJob Ordner Struktur
	//

	//! Gibt true zur�ck, falls this ein Ordner ist
	bool isDirectory();

	//! Ersetzt im Pfad von this ab pos die folgenden n Zeichen durch newPath
	/*!
	 *  Gedacht zum Aufruf auf ein Ordner Item, um den Ordner umzubennenen.
	 */
	void updatePath(int pos, int n, QString newPath);

	//! Bev�lkert den Baum
	/*!
	 *  Kann Dateien nur in einem tieferen Verzeichnis als sich selbst einf�gen und sollte
	 *  entsprechend nur auf die Wurzel aufgerufen werden.
	 *  @param fileList enth�lt eine Liste von Dateien und ihre Eigenschaften.
	 *
	 *  Gibt die Anzahl der hinzugef�gten Dateien zur�ck.
	 */
        int populateTree(QList<QList<QVariant> > fileList);

	//! Formatiert Dateieigenschaften einer Datei aus dem PJobFileFormat f�r die GUI.
	/*!
	 *  Formatiert den Datensatz aus Dateiname, �nderungszeit und Dateigr��e aus
	 *  dem PJobFileFormat in entsprechende Strings f�r die GUI.
	 */
	static QList<QVariant> formatDataToString(const QList<QVariant>& fileProperties);

protected:
	void appendItem(PJobDirTree* item);

	//! F�gt anhand der Tiefe der Verzeichnisse die Dateien aus fileList ein.
	//! Siehe populateTree(QList<QList<QVariant>> fileList).
        int populateTreeRec(QList<QList<QVariant> > fileList, PJobDirTree* currentItem,
						QString currentDir, int depth, int index = 0);

	PJobDirTree* m_parent; 
	QList<PJobDirTree*> m_children;
	QList<QVariant> m_data;
};
