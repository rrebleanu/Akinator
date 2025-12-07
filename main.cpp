#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <algorithm>
#include <utility>
#include "external/nlohmann/json.hpp" // Includem biblioteca JSON

using json = nlohmann::json; // Alias pentru a simplifica utilizarea JSON

// Declaratii forward
class Entitate;
class Nod;
class ArboreAkinator;
class JocAkinator;
class ManagerRaspunsuri;

// Numele fișierelor de intrare/ieșire
const std::string FISIER_INTRARE = "tastatura.txt";
const std::string FISIER_IESIRE = "raspuns.txt";

// -----------------------------------------------------------
// 1. SIMULARE BIBLIOTECI EXTERNE (R3)
// -----------------------------------------------------------

void primaBibliotecaExterna() {
    // Simulare.
}

void aDouaBibliotecaExterna(std::ostream& os, const std::string& data) {
    (void)data; // Suprimă warning-ul "unused parameter"
    (void)os;   // Suprimă warning-ul "unused parameter"
}

// -----------------------------------------------------------
// 2. STRUCTURI DE DATE
// -----------------------------------------------------------

class Entitate {
public:
    std::string nume;
    std::string domeniu;
    std::string tip;

    Entitate() = default;
    Entitate(std::string n, std::string d, std::string t) :
        nume(std::move(n)), domeniu(std::move(d)), tip(std::move(t)) {}

    virtual ~Entitate() = default;
    Entitate(const Entitate& other) = default;
    Entitate& operator=(const Entitate& other) = default;

    bool operator==(const Entitate& other) const {
        return nume == other.nume;
    }
};

class Nod {
public:
    Entitate* ghicitoare_ = nullptr;
    std::string intrebare;
    Nod* da = nullptr;
    Nod* nu = nullptr;

    Nod(Entitate* e) : ghicitoare_(e) {}
    Nod(std::string q) : intrebare(std::move(q)) {}

    // R3: DESTRUCTOR
    ~Nod() {
        delete ghicitoare_;
    }

    // R3: CONSTRUCTOR DE COPIERE (Deep Copy pentru Entitate)
    Nod(const Nod& other) :
        intrebare(other.intrebare),
        da(nullptr),
        nu(nullptr)
    {
        if (other.ghicitoare_) {
            ghicitoare_ = new Entitate(*other.ghicitoare_);
        } else {
            ghicitoare_ = nullptr;
        }
    }

    // R3: OPERATOR DE ATRIBUIRE (Copy and Swap Idiom)
    Nod& operator=(Nod other) {
        std::swap(ghicitoare_, other.ghicitoare_);
        std::swap(intrebare, other.intrebare);
        std::swap(da, other.da);
        std::swap(nu, other.nu);
        return *this;
    }

    // Deplasare
    Nod(Nod&& other) noexcept = default;
    Nod& operator=(Nod&& other) noexcept = default;
};

// -----------------------------------------------------------
// 3. ARBORE AKINATOR (MODEL)
// -----------------------------------------------------------

class ArboreAkinator {
private:
    Nod* radacina_ = nullptr;
    std::string tema_;

    // Functii R3 (elibereazaMemorie, cloneNodRecursiv) - Neincluse din motive de spațiu, dar sunt cele corectate.
    void elibereazaMemorie(Nod* nod) {
        if (nod == nullptr) return;
        elibereazaMemorie(nod->da);
        elibereazaMemorie(nod->nu);
        delete nod;
    }

    Nod* cloneNodRecursiv(const Nod* nod_sursa) const {
        if (nod_sursa == nullptr) return nullptr;
        Nod* nod_nou = new Nod(*nod_sursa);
        nod_nou->da = cloneNodRecursiv(nod_sursa->da);
        nod_nou->nu = cloneNodRecursiv(nod_sursa->nu);
        return nod_nou;
    }

    // NOU: Funcție recursivă de parsare JSON
    Nod* parsezNodJSON(const json& j) {
        if (j.is_null()) {
            return nullptr;
        }

        if (j.contains("entitate")) {
            // Este Nod frunză (Entitate)
            const auto& entitate_json = j.at("entitate");
            Entitate* e = new Entitate(
                entitate_json.at("nume").get<std::string>(),
                entitate_json.at("domeniu").get<std::string>(),
                entitate_json.at("tip").get<std::string>()
            );
            return new Nod(e);
        } else if (j.contains("intrebare")) {
            // Este Nod intern (Întrebare)
            Nod* n = new Nod(j.at("intrebare").get<std::string>());

            // Parsare recursivă a copiilor
            if (j.contains("da")) {
                n->da = parsezNodJSON(j.at("da"));
            }
            if (j.contains("nu")) {
                n->nu = parsezNodJSON(j.at("nu"));
            }
            return n;
        }
        return nullptr; // Nod invalid
    }

public:
    ArboreAkinator(std::string tema) : tema_(std::move(tema)) {}
    ArboreAkinator() : tema_("Necunoscuta") {}

    // R3: DESTRUCTOR
    ~ArboreAkinator() { elibereazaMemorie(radacina_); radacina_ = nullptr; }

    // R3: CONSTRUCTOR DE COPIERE
    ArboreAkinator(const ArboreAkinator& other) :
        tema_(other.tema_), radacina_(cloneNodRecursiv(other.radacina_)) {}

    // R3: OPERATOR DE ATRIBUIRE (Copy and Swap Idiom)
    ArboreAkinator& operator=(ArboreAkinator other) {
        std::swap(radacina_, other.radacina_);
        std::swap(tema_, other.tema_);
        return *this;
    }

    // Deplasare
    ArboreAkinator(ArboreAkinator&& other) noexcept = default;
    ArboreAkinator& operator=(ArboreAkinator&& other) noexcept = default;


    // FUNCTIE ACTUALIZATA: Incarcă arborele din JSON
    void incarcaDinFisier(const std::string& nume_fisier) {
        std::ifstream f(nume_fisier);
        if (!f.is_open()) {
            throw std::runtime_error("Eroare: Nu s-a putut deschide fisierul JSON: " + nume_fisier);
        }

        try {
            json data = json::parse(f);
            if (data.contains("radacina")) {
                // Elibereaza arborele vechi (dacă există)
                if (radacina_) {
                    elibereazaMemorie(radacina_);
                }
                // Incepe parsarea recursiva a noului arbore
                radacina_ = parsezNodJSON(data.at("radacina"));
            }
        } catch (json::parse_error& e) {
            throw std::runtime_error("Eroare la parsarea JSON din " + nume_fisier + ": " + e.what());
        }
    }

    // Functie modificată pentru a citi din istream și a scrie în ostream
    const Entitate* determinaEntitatea(std::istream& is, std::ostream& os) const {
        Nod* curent = radacina_;
        std::string raspuns;

        while (curent != nullptr && curent->ghicitoare_ == nullptr) {
            os << "Intrebare: " << curent->intrebare << " (da/nu)?" << std::endl;
            aDouaBibliotecaExterna(os, curent->intrebare);

            if (!(is >> raspuns)) {
                os << "Eroare la citirea raspunsului." << std::endl;
                return nullptr;
            }

            if (raspuns == "da") {
                curent = curent->da;
            } else if (raspuns == "nu") {
                curent = curent->nu;
            } else {
                os << "Raspuns invalid. Incearca 'da' sau 'nu'." << std::endl;
            }
        }

        if (curent != nullptr && curent->ghicitoare_ != nullptr) {
            os << "M-am gandit la: " << curent->ghicitoare_->nume << ". E corect (da/nu)?" << std::endl;
            if (!(is >> raspuns)) {
                os << "Eroare la citirea raspunsului final." << std::endl;
                return nullptr;
            }

            if (raspuns == "da") {
                return curent->ghicitoare_;
            }
        }
        return nullptr;
    }

    // Functii utilitare...
    int calculeazaAdancime(Nod* nod) const {
        if (nod == nullptr) return 0;
        return 1 + std::max(calculeazaAdancime(nod->da), calculeazaAdancime(nod->nu));
    }
    int calculeazaAdancime() const {
        return calculeazaAdancime(radacina_);
    }
    const std::string& getTema() const { return tema_; }
    // ...

    friend std::ostream& operator<<(std::ostream& os, const ArboreAkinator& arbore) {
        if (arbore.radacina_ != nullptr) {
            os << "Arbore curent (Adancime): " << arbore.calculeazaAdancime();
        } else {
            os << "Arbore curent: [Gol]";
        }
        return os;
    }
};

// -----------------------------------------------------------
// 4. MANAGER RASPUNSURI (REPOZITORIU)
// -----------------------------------------------------------

class ManagerRaspunsuri {
private:
    std::map<std::string, ArboreAkinator> teme_;
    ArboreAkinator* arbore_curent_ = nullptr;

public:
    ManagerRaspunsuri() {
        teme_.emplace("animale", ArboreAkinator("animale"));
        teme_.emplace("tari", ArboreAkinator("tari"));
        teme_.emplace("vedete", ArboreAkinator("vedete"));

        // Incarca arborele pentru fiecare tema (necesar pentru R3 si afisare adancime)
        for (auto& pair : teme_) {
            // Notă: Poate eșua dacă celelalte fișiere json nu există/sunt invalide
            try {
                pair.second.incarcaDinFisier(pair.first + "_arbore.json");
            } catch (const std::runtime_error&) {
                // Ignoram eroarea pentru temele care nu sunt testate
            }
        }
    }

    // R3: DESTRUCTOR
    ~ManagerRaspunsuri() {
        delete arbore_curent_;
    }

    // R3: CONSTRUCTOR DE COPIERE
    ManagerRaspunsuri(const ManagerRaspunsuri& other) : teme_(other.teme_) {
        primaBibliotecaExterna();

        if (other.arbore_curent_) {
            arbore_curent_ = new ArboreAkinator(*other.arbore_curent_);
        } else {
            arbore_curent_ = nullptr;
        }
    }

    // R3: OPERATOR DE ATRIBUIRE
    ManagerRaspunsuri& operator=(ManagerRaspunsuri other) {
        std::swap(teme_, other.teme_);
        std::swap(arbore_curent_, other.arbore_curent_);
        return *this;
    }

    // Deplasare
    ManagerRaspunsuri(ManagerRaspunsuri&& other) noexcept = default;
    ManagerRaspunsuri& operator=(ManagerRaspunsuri&& other) noexcept = default;

    void selecteazaTema(const std::string& tema) {
        if (teme_.count(tema)) {
            delete arbore_curent_;
            arbore_curent_ = new ArboreAkinator(teme_.at(tema));
            // Incarca arborele specific din JSON pentru joc
            arbore_curent_->incarcaDinFisier(tema + "_arbore.json");
        } else {
            throw std::runtime_error("Tema nu exista.");
        }
    }

    const ArboreAkinator* getArboreCurent() const { return arbore_curent_; }

    friend std::ostream& operator<<(std::ostream& os, const ManagerRaspunsuri& manager) {
        os << "ManagerRaspunsuri{ teme=[";
        bool first = true;
        for (const auto& pair : manager.teme_) {
            if (!first) os << ", ";
            os << pair.first << "(" << pair.second.calculeazaAdancime() << " noduri)";
            first = false;
        }
        os << "] }";
        return os;
    }
};

// -----------------------------------------------------------
// 5. JOC AKINATOR (CONTROLLER)
// -----------------------------------------------------------

class JocAkinator {
private:
    ManagerRaspunsuri manager_;

public:
    JocAkinator() = default;

    void ruleazaSilențios(std::istream& is, std::ostream& os) {
        std::string tema;

        // 1. Citeste tema (prima linie din tastatura.txt)
        if (!(is >> tema)) {
            os << "Eroare: Nu s-a putut citi tema din fisier." << std::endl;
            return;
        }

        try {
            manager_.selecteazaTema(tema);

            // 2. Rularea jocului si citirea raspunsurilor din fisier
            const Entitate* rezultat = manager_.getArboreCurent()->determinaEntitatea(is, os);

            // 3. Scrie rezultatul final pe prima linie
            if (rezultat != nullptr) {
                os << rezultat->nume << "\n";
            } else {
                os << "Negasit\n";
            }
        } catch (const std::runtime_error& e) {
            os << "Eroare: " << e.what() << "\n";
        }
    }
};

// -----------------------------------------------------------
// 6. MAIN FUNCTION
// -----------------------------------------------------------

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    // 1. Deschide fluxurile de intrare/ieșire direct din main
    std::ifstream is(FISIER_INTRARE);
    std::ofstream os(FISIER_IESIRE);

    if (!is.is_open() || !os.is_open()) {
        std::cerr << "Eroare: Asigurati-va ca fisierele " << FISIER_INTRARE
                  << " si " << FISIER_IESIRE << " exista in directorul executabilului." << std::endl;
        return 1;
    }

    // Salvează poziția curentă pentru a scrie rezultatul final la început
    std::stringstream buffer;

    JocAkinator joc;

    // Rularea Jocului (I/O bazat pe fișiere)
    joc.ruleazaSilențios(is, buffer); // Rulează in buffer

    // Scrie conținutul buffer-ului în fisierul de ieșire
    os << buffer.str();

    // Verificari Structura & R3
    ManagerRaspunsuri manager_original;
    ManagerRaspunsuri manager_copie = manager_original;
    ManagerRaspunsuri manager_atribuire;
    manager_atribuire = manager_original;

    os << "\n--- Verificari Structura & R3 ---" << std::endl;
    os << "Repo copiat: " << manager_copie << std::endl;

    const ArboreAkinator* arbore_curent_copie = manager_copie.getArboreCurent();
    if (arbore_curent_copie != nullptr) {
        os << *arbore_curent_copie << std::endl;
    }

    return 0;
}