#include "core/bible_verses.hpp"

#include <array>

namespace ortha {

namespace {

// Luther 1912 (public domain), German verse numbering. Passages join their verses.
constexpr std::array kVerses = std::to_array<BibleVerse>({
    {"Johannes 3,16",
     "Also hat Gott die Welt geliebt, daß er seinen eingeborenen Sohn gab, auf daß"
     " alle, die an ihn glauben, nicht verloren werden, sondern das ewige Leben "
     "haben."},
    {"Römer 5,8",
     "Darum preiset Gott seine Liebe gegen uns, daß Christus für uns gestorben "
     "ist, da wir noch Sünder waren."},
    {"1. Johannes 4,8",
     "Wer nicht liebhat, der kennt Gott nicht; denn Gott ist Liebe."},
    {"1. Johannes 4,19",
     "Lasset uns ihn lieben; denn er hat uns zuerst geliebt."},
    {"Epheser 2,8",
     "Denn aus Gnade seid ihr selig geworden durch den Glauben, und das nicht aus "
     "euch: Gottes Gabe ist es,"},
    {"Römer 8,38–39",
     "Denn ich bin gewiß, daß weder Tod noch Leben, weder Engel noch Fürstentümer "
     "noch Gewalten, weder Gegenwärtiges noch Zukünftiges, weder Hohes noch Tiefes"
     " noch keine andere Kreatur mag uns scheiden von der Liebe Gottes, die in "
     "Christo Jesu ist, unserm HERRN."},
    {"Klagelieder 3,22–23",
     "Die Güte des HERRN ist's, daß wir nicht gar aus sind; seine Barmherzigkeit "
     "hat noch kein Ende, sondern sie ist alle Morgen neu, und deine Treue ist "
     "groß."},
    {"Psalm 103,8",
     "Barmherzig und gnädig ist der HERR, geduldig und von großer Güte."},
    {"Psalm 136,1",
     "Danket dem HERRN; denn er ist freundlich, denn seine Güte währet ewiglich."},
    {"Titus 3,5",
     "nicht um der Werke willen der Gerechtigkeit, die wir getan hatten, sondern "
     "nach seiner Barmherzigkeit machte er uns selig durch das Bad der "
     "Wiedergeburt und Erneuerung des heiligen Geistes,"},
    {"Johannes 14,6",
     "Jesus spricht zu ihm: Ich bin der Weg und die Wahrheit und das Leben; "
     "niemand kommt zum Vater denn durch mich."},
    {"Johannes 11,25",
     "Jesus spricht zu ihr: Ich bin die Auferstehung und das Leben. Wer an mich "
     "glaubt, der wird leben, ob er gleich stürbe;"},
    {"Matthäus 11,28",
     "Kommet her zu mir alle, die ihr mühselig und beladen seid; ich will euch "
     "erquicken."},
    {"Markus 10,45",
     "Denn auch des Menschen Sohn ist nicht gekommen, daß er sich dienen lasse, "
     "sondern daß er diene und gebe sein Leben zur Bezahlung für viele."},
    {"Apostelgeschichte 4,12",
     "Und ist in keinem andern Heil, ist auch kein andrer Name unter dem Himmel "
     "den Menschen gegeben, darin wir sollen selig werden."},
    {"Römer 10,9",
     "Denn so du mit deinem Munde bekennst Jesum, daß er der HERR sei, und glaubst"
     " in deinem Herzen, daß ihn Gott von den Toten auferweckt hat, so wirst du "
     "selig."},
    {"Römer 6,23",
     "Denn der Tod ist der Sünde Sold; aber die Gabe Gottes ist das ewige Leben in"
     " Christo Jesu, unserm HERRN."},
    {"2. Korinther 5,17",
     "Darum, ist jemand in Christo, so ist er eine neue Kreatur; das Alte ist "
     "vergangen, siehe, es ist alles neu geworden!"},
    {"Galater 2,20",
     "Ich lebe aber; doch nun nicht ich, sondern Christus lebt in mir. Denn was "
     "ich jetzt lebe im Fleisch, das lebe ich in dem Glauben des Sohnes Gottes, "
     "der mich geliebt hat und sich selbst für mich dargegeben."},
    {"Philipper 1,21",
     "Denn Christus ist mein Leben, und Sterben ist mein Gewinn."},
    {"Sprüche 3,5–6",
     "Verlaß dich auf den HERRN von ganzem Herzen und verlaß dich nicht auf deinen"
     " Verstand; sondern gedenke an ihn in allen deinen Wegen, so wird er dich "
     "recht führen."},
    {"Hebräer 11,1",
     "Es ist aber der Glaube eine gewisse Zuversicht des, das man hofft, und ein "
     "Nichtzweifeln an dem, das man nicht sieht."},
    {"Markus 11,24",
     "Darum sage ich euch: Alles, was ihr bittet in eurem Gebet, glaubet nur, daß "
     "ihr's empfangen werdet, so wird's euch werden."},
    {"Philipper 4,13",
     "Ich vermag alles durch den, der mich mächtig macht, Christus."},
    {"Psalm 37,5",
     "Befiehl dem HERRN deine Wege und hoffe auf ihn; er wird's wohl machen"},
    {"Psalm 56,4",
     "Wenn ich mich fürchte, so hoffe ich auf dich."},
    {"Jeremia 17,7",
     "Gesegnet aber ist der Mann, der sich auf den HERRN verläßt und des "
     "Zuversicht der HERR ist."},
    {"2. Korinther 5,7",
     "denn wir wandeln im Glauben, und nicht im Schauen."},
    {"Römer 10,17",
     "So kommt der Glaube aus der Predigt, das Predigen aber aus dem Wort Gottes."},
    {"Hebräer 11,6",
     "Aber ohne Glauben ist's unmöglich, Gott zu gefallen; denn wer zu Gott kommen"
     " will, der muß glauben, daß er sei und denen, die ihn suchen, ein Vergelter "
     "sein werde."},
    {"Philipper 4,6–7",
     "Sorget nichts! sondern in allen Dingen lasset eure Bitten im Gebet und "
     "Flehen mit Danksagung vor Gott kund werden. Und der Friede Gottes, welcher "
     "höher ist denn alle Vernunft, bewahre eure Herzen und Sinne in Christo Jesu!"},
    {"1. Petrus 5,7",
     "Alle Sorge werfet auf ihn; denn er sorgt für euch."},
    {"Jesaja 41,10",
     "fürchte dich nicht, ich bin mit dir; weiche nicht, denn ich bin dein Gott; "
     "ich stärke dich, ich helfe dir auch, ich erhalte dich durch die rechte Hand "
     "meiner Gerechtigkeit."},
    {"Josua 1,9",
     "Siehe, ich habe dir geboten, daß du getrost und freudig seist. Laß dir nicht"
     " grauen und entsetze dich nicht; denn der HERR, dein Gott, ist mit dir in "
     "allem, was du tun wirst."},
    {"Psalm 23,4",
     "Und ob ich schon wanderte im finstern Tal, fürchte ich kein Unglück; denn du"
     " bist bei mir, dein Stecken und dein Stab trösten mich."},
    {"Psalm 46,2",
     "Gott ist unsre Zuversicht und Stärke. Eine Hilfe in den großen Nöten, die "
     "uns getroffen haben."},
    {"Matthäus 6,34",
     "Darum sorgt nicht für den andern Morgen; denn der morgende Tag wird für das "
     "Seine sorgen. Es ist genug, daß ein jeglicher Tag seine eigene Plage habe."},
    {"Johannes 14,27",
     "Den Frieden lasse ich euch, meinen Frieden gebe ich euch. Nicht gebe ich "
     "euch, wie die Welt gibt. Euer Herz erschrecke nicht und fürchte sich nicht."},
    {"Psalm 34,5",
     "Da ich den HERRN suchte, antwortete er mir und errettete mich aus aller "
     "meiner Furcht."},
    {"Psalm 55,23",
     "Wirf dein Anliegen auf den HERRN; der wird dich versorgen und wird den "
     "Gerechten nicht ewiglich in Unruhe lassen."},
    {"Jesaja 40,31",
     "aber die auf den HERRN harren, kriegen neue Kraft, daß sie auffahren mit "
     "Flügeln wie Adler, daß sie laufen und nicht matt werden, daß sie wandeln und"
     " nicht müde werden."},
    {"Psalm 31,25",
     "Seid getrost und unverzagt, alle, die ihr des HERRN harret!"},
    {"Römer 15,13",
     "Der Gott aber der Hoffnung erfülle euch mit aller Freude und Frieden im "
     "Glauben, daß ihr völlige Hoffnung habet durch die Kraft des heiligen "
     "Geistes."},
    {"Jeremia 29,11",
     "Denn ich weiß wohl, was ich für Gedanken über euch habe, spricht der HERR: "
     "Gedanken des Friedens und nicht des Leidens, daß ich euch gebe das Ende, des"
     " ihr wartet."},
    {"Psalm 121,1–2",
     "Ein Lied im höhern Chor. Ich hebe meine Augen auf zu den Bergen von welchen "
     "mir Hilfe kommt. Meine Hilfe kommt von dem HERRN, der Himmel und Erde "
     "gemacht hat."},
    {"Psalm 27,1",
     "Ein Psalm Davids. Der HERR ist mein Licht und mein Heil; vor wem sollte ich "
     "mich fürchten! Der HERR ist meines Lebens Kraft; vor wem sollte mir grauen!"},
    {"2. Timotheus 1,7",
     "Denn Gott hat uns nicht gegeben den Geist der Furcht, sondern der Kraft und "
     "der Liebe und der Zucht."},
    {"Nehemia 8,10",
     "Darum sprach er zu ihnen: Geht hin und eßt das Fette und trinkt das Süße und"
     " sendet denen auch Teile, die nichts für sich bereitet haben; denn dieser "
     "Tag ist heilig unserm HERRN. Und bekümmert euch nicht; denn die Freude am "
     "HERRN ist eure Stärke."},
    {"Psalm 18,3",
     "HERR, mein Fels, meine Burg, mein Erretter, mein Gott, mein Hort, auf den "
     "ich traue, mein Schild und Horn meines Heils und mein Schutz!"},
    {"Habakuk 3,17–18",
     "Denn der Feigenbaum wird nicht grünen, und wird kein Gewächs sein an den "
     "Weinstöcken; die Arbeit am Ölbaum ist vergeblich, und die Äcker bringen "
     "keine Nahrung; und Schafe werden aus den Hürden gerissen, und werden keine "
     "Rinder in den Ställen sein. Aber ich will mich freuen des HERRN und fröhlich"
     " sein in Gott, meinem Heil."},
    {"Matthäus 22,37–39",
     "Jesus aber sprach zu ihm: \"Du sollst lieben Gott, deinen HERRN, von ganzem "
     "Herzen, von ganzer Seele und von ganzem Gemüte.\" Dies ist das vornehmste und"
     " größte Gebot. Das andere aber ist ihm gleich: \"Du sollst deinen Nächsten "
     "lieben wie dich selbst.\""},
    {"1. Korinther 13,4–7",
     "Die Liebe ist langmütig und freundlich, die Liebe eifert nicht, die Liebe "
     "treibt nicht Mutwillen, sie blähet sich nicht, sie stellet sich nicht "
     "ungebärdig, sie suchet nicht das Ihre, sie läßt sich nicht erbittern, sie "
     "rechnet das Böse nicht zu, sie freut sich nicht der Ungerechtigkeit, sie "
     "freut sich aber der Wahrheit; sie verträgt alles, sie glaubet alles, sie "
     "hoffet alles, sie duldet alles."},
    {"Johannes 13,34",
     "Ein neu Gebot gebe ich euch, daß ihr euch untereinander liebet, wie ich euch"
     " geliebt habe, auf daß auch ihr einander liebhabet."},
    {"1. Korinther 16,14",
     "Alle eure Dinge lasset in der Liebe geschehen!"},
    {"Kolosser 3,14",
     "Über alles aber ziehet an die Liebe, die da ist das Band der Vollkommenheit."},
    {"Epheser 4,32",
     "Seid aber untereinander freundlich, herzlich und vergebet einer dem andern, "
     "gleichwie Gott euch auch vergeben hat in Christo."},
    {"Römer 12,10",
     "Die brüderliche Liebe untereinander sei herzlich. Einer komme dem andern mit"
     " Ehrerbietung zuvor."},
    {"Galater 6,2",
     "Einer trage des andern Last, so werdet ihr das Gesetz Christi erfüllen."},
    {"Matthäus 5,44",
     "Ich aber sage euch: Liebet eure Feinde; segnet, die euch fluchen; tut wohl "
     "denen, die euch hassen; bittet für die, so euch beleidigen und verfolgen,"},
    {"Lukas 6,31",
     "Und wie ihr wollt, daß euch die Leute tun sollen, also tut ihnen gleich auch"
     " ihr."},
    {"Matthäus 6,9–13",
     "Darum sollt ihr also beten: Unser Vater in dem Himmel! Dein Name werde "
     "geheiligt. Dein Reich komme. Dein Wille geschehe auf Erden wie im Himmel. "
     "Unser täglich Brot gib uns heute. Und vergib uns unsere Schuld, wie wir "
     "unseren Schuldigern vergeben. Und führe uns nicht in Versuchung, sondern "
     "erlöse uns von dem Übel. Denn dein ist das Reich und die Kraft und die "
     "Herrlichkeit in Ewigkeit. Amen."},
    {"Jeremia 33,3",
     "Rufe mich an, so will ich dir antworten und will dir anzeigen große und "
     "gewaltige Dinge, die du nicht weißt."},
    {"1. Thessalonicher 5,16–18",
     "Seid allezeit fröhlich, betet ohne Unterlaß, seid dankbar in allen Dingen; "
     "denn das ist der Wille Gottes in Christo Jesu an euch."},
    {"Markus 11,25",
     "Und wenn ihr stehet und betet, so vergebet, wo ihr etwas wider jemand habt, "
     "auf daß auch euer Vater im Himmel euch vergebe eure Fehler."},
    {"Johannes 15,7",
     "So ihr in mir bleibet und meine Worte in euch bleiben, so werdet ihr bitten,"
     " was ihr wollt, und es wird euch widerfahren."},
    {"Psalm 50,15",
     "und rufe mich an in der Not, so will ich dich erretten, so sollst du mich "
     "preisen."},
    {"Jakobus 5,16",
     "Bekenne einer dem andern seine Sünden und betet füreinander, daß ihr gesund "
     "werdet. Des Gerechten Gebet vermag viel, wenn es ernstlich ist."},
    {"Lukas 18,1",
     "Er sagte ihnen aber ein Gleichnis davon, daß man allezeit beten und nicht "
     "laß werden solle,"},
    {"Philipper 4,6",
     "Sorget nichts! sondern in allen Dingen lasset eure Bitten im Gebet und "
     "Flehen mit Danksagung vor Gott kund werden."},
    {"Psalm 145,18",
     "Der HERR ist nahe allen, die ihn anrufen, allen, die ihn mit Ernst anrufen."},
    {"Psalm 119,105",
     "Dein Wort ist meines Fußes Leuchte und ein Licht auf meinem Wege."},
    {"Sprüche 4,23",
     "Behüte dein Herz mit allem Fleiß; denn daraus geht das Leben."},
    {"Sprüche 16,3",
     "Befiehl dem HERRN deine Werke, so werden deine Anschläge fortgehen."},
    {"Sprüche 16,9",
     "Des Menschen Herz erdenkt sich seinen Weg; aber der HERR allein gibt, daß er"
     " fortgehe."},
    {"Psalm 1,1–3",
     "Wohl dem, der nicht wandelt im Rat der Gottlosen noch tritt auf den Weg der "
     "Sünder noch sitzt, da die Spötter sitzen, sondern hat Lust zum Gesetz des "
     "HERRN und redet von seinem Gesetz Tag und Nacht! Der ist wie ein Baum, "
     "gepflanzt an den Wasserbächen, der seine Frucht bringt zu seiner Zeit, und "
     "seine Blätter verwelken nicht; und was er macht, das gerät wohl."},
    {"Micha 6,8",
     "Es ist dir gesagt, Mensch, was gut ist und was der HERR von dir fordert, "
     "nämlich Gottes Wort halten und Liebe üben und demütig sein vor deinem Gott."},
    {"Prediger 3,1",
     "Ein jegliches hat seine Zeit, und alles Vornehmen unter dem Himmel hat seine"
     " Stunde."},
    {"Sprüche 15,1",
     "Eine linde Antwort stillt den Zorn; aber ein hartes Wort richtet Grimm an."},
    {"Sprüche 27,17",
     "Ein Messer wetzt das andere und ein Mann den andern."},
    {"Jakobus 1,5",
     "So aber jemand unter euch Weisheit mangelt, der bitte Gott, der da gibt "
     "einfältig jedermann und rücket's niemand auf, so wird sie ihm gegeben "
     "werden."},
    {"1. Johannes 1,9",
     "So wir aber unsre Sünden bekennen, so ist er treu und gerecht, daß er uns "
     "die Sünden vergibt und reinigt uns von aller Untugend."},
    {"Psalm 103,12",
     "So ferne der Morgen ist vom Abend, läßt er unsre Übertretungen von uns sein."},
    {"Jesaja 1,18",
     "So kommt denn und laßt uns miteinander rechten, spricht der HERR. Wenn eure "
     "Sünde gleich blutrot ist, soll sie doch schneeweiß werden; und wenn sie "
     "gleich ist wie Scharlach, soll sie doch wie Wolle werden."},
    {"Psalm 51,12",
     "Schaffe in mir, Gott, ein reines Herz und gib mir einen neuen, gewissen "
     "Geist."},
    {"Lukas 15,7",
     "Ich sage euch: Also wird auch Freude im Himmel sein über einen Sünder, der "
     "Buße tut, vor neunundneunzig Gerechten, die der Buße nicht bedürfen."},
    {"2. Chronik 7,14",
     "und mein Volk sich demütigt, das nach meinem Namen genannt ist, daß sie "
     "beten und mein Angesicht suchen und sich von ihren bösen Wegen bekehren "
     "werden: so will ich vom Himmel hören und ihre Sünde vergeben und ihr Land "
     "heilen."},
    {"Epheser 1,7",
     "an welchem wir haben die Erlösung durch sein Blut, die Vergebung der Sünden,"
     " nach dem Reichtum seiner Gnade,"},
    {"Kolosser 3,13",
     "und vertrage einer den andern und vergebet euch untereinander, so jemand "
     "Klage hat wider den andern; gleichwie Christus euch vergeben hat, also auch "
     "ihr."},
    {"Lukas 23,34",
     "Jesus aber sprach: Vater, vergib ihnen, sie wissen nicht, was sie tun! Und "
     "sie teilten seine Kleider und warfen das Los darum."},
    {"Psalm 32,1",
     "Eine Unterweisung Davids. Wohl dem, dem die Übertretungen vergeben sind, dem"
     " die Sünde bedeckt ist!"},
    {"Johannes 3,36",
     "Wer an den Sohn glaubt, der hat das ewige Leben. Wer dem Sohn nicht glaubt, "
     "der wird das Leben nicht sehen, sondern der Zorn Gottes bleibt über ihm."},
    {"Johannes 14,2–3",
     "In meines Vaters Hause sind viele Wohnungen. Wenn es nicht so wäre, so "
     "wollte ich zu euch sagen: Ich gehe hin euch die Stätte zu bereiten. Und wenn"
     " ich hingehe euch die Stätte zu bereiten, so will ich wiederkommen und euch "
     "zu mir nehmen, auf daß ihr seid, wo ich bin."},
    {"Offenbarung 21,4",
     "und Gott wird abwischen alle Tränen von ihren Augen, und der Tod wird nicht "
     "mehr sein, noch Leid noch Geschrei noch Schmerz wird mehr sein; denn das "
     "Erste ist vergangen."},
    {"Römer 8,18",
     "Denn ich halte es dafür, daß dieser Zeit Leiden der Herrlichkeit nicht wert "
     "sei, die an uns soll offenbart werden."},
    {"1. Korinther 15,55",
     "\"Der Tod ist verschlungen in den Sieg. Tod, wo ist dein Stachel? Hölle, wo "
     "ist dein Sieg?\""},
    {"1. Thessalonicher 4,16–17",
     "denn er selbst, der HERR, wird mit einem Feldgeschrei und der Stimme des "
     "Erzengels und mit der Posaune Gottes herniederkommen vom Himmel, und die "
     "Toten in Christo werden auferstehen zuerst. Darnach wir, die wir leben und "
     "übrig bleiben, werden zugleich mit ihnen hingerückt werden in den Wolken, "
     "dem HERRN entgegen in der Luft, und werden also bei dem HERRN sein allezeit."},
    {"Philipper 3,20",
     "Unser Wandel aber ist im Himmel, von dannen wir auch warten des Heilands "
     "Jesu Christi, des HERRN,"},
    {"Titus 2,13",
     "und warten auf die selige Hoffnung und Erscheinung der Herrlichkeit des "
     "großen Gottes und unsers Heilandes, Jesu Christi,"},
    {"Offenbarung 22,20",
     "Es spricht, der solches bezeugt: Ja, ich komme bald. Amen, ja komm, HERR "
     "Jesu!"},
    {"4. Mose 6,24–26",
     "Der HERR segne dich und behüte dich; der HERR lasse sein Angesicht leuchten "
     "über dir und sei dir gnädig; der HERR hebe sein Angesicht über dich und gebe"
     " dir Frieden."},
});

} // namespace

std::span<const BibleVerse> bible_verses() { return kVerses; }

} // namespace ortha
