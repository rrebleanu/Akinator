#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <utility>
#include <map>
#include <vector>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <cctype>

// Biblioteca externă nlohmann/json este folosită pentru funcțiile din JSON
#include "external/nlohmann/json.hpp"
using json = nlohmann::json;
using namespace std;

// DECLARATII FORWARD & PLACEHOLDER BIBLIOTECI EXTERNE
class Entitate; class Nod; class ArboreAkinator; class ManagerRaspunsuri; class JocAkinator;

// Simulare a celei de-a doua biblioteci externe (Logging/Sanitizare)
// void aDouaBibliotecaExterna(const std::string& data) {
//     (void)data;
//     // Apelată la fiecare întrebare. Nu face nimic in acest scenariu.
// }

// CLASA 1: Entitate
class Entitate {
private:
    std::string nume_;
    std::string domeniu_;
    std::string tip_entitate_;
public:
    // Constructor de inițializare
    Entitate(const std::string& nume, const std::string& domeniu, const std::string& tip)
        : nume_(nume), domeniu_(domeniu), tip_entitate_(tip) {}

    // Regula Celor Trei (R3): Constructor de Copiere, Operator=, Destructor
    Entitate(const Entitate& other) = default;
    Entitate& operator=(const Entitate& other) = default;
    ~Entitate() = default;

    [[nodiscard]] const std::string& getNume() const noexcept { return nume_; }

    // Operator<<
    friend std::ostream& operator<<(std::ostream& os, const Entitate& e);
};

std::ostream& operator<<(std::ostream& os, const Entitate& e) {
    os << "Entitate{ nume='" << e.nume_ << "', domeniu='" << e.domeniu_
       << "', tip='" << e.tip_entitate_ << "' }";
    return os;
}

// CLASA 2: Nod (Structura Arborelui, Compunere)
class Nod {
private:
    std::string intrebare_;
    std::unique_ptr<Nod> da_; // Ramura DA (Smart Pointer)
    std::unique_ptr<Nod> nu_; // Ramura NU (Smart Pointer)
    Entitate* ghicitoare_ = nullptr; // Pointer raw catre Entitate (gestionat de Nod)
public:
    // Constructori
    explicit Nod(std::unique_ptr<Entitate> entitate_ghicita) : ghicitoare_(entitate_ghicita.release()) {}
    Nod(const std::string& intrebare, std::unique_ptr<Nod> da, std::unique_ptr<Nod> nu, Entitate* ghicitoare)
        : intrebare_(intrebare), da_(std::move(da)), nu_(std::move(nu)), ghicitoare_(ghicitoare) {}
    ~Nod() { delete ghicitoare_; } // Sterge Entitatea
    Nod(const Nod& other) = delete; Nod& operator=(const Nod& other) = delete; // Copierea este interzisa

    // Getters
    [[nodiscard]] const std::string& getIntrebare() const { return intrebare_; }
    [[nodiscard]] Nod* getDa() const { return da_.get(); }
    [[nodiscard]] Nod* getNu() const { return nu_.get(); }
    [[nodiscard]] Entitate* getGhicitoare() const { return ghicitoare_; }
    [[nodiscard]] bool esteFrunza() const { return da_ == nullptr && nu_ == nullptr; }

    // Operator<<
    friend std::ostream& operator<<(std::ostream& os, const Nod& n);
    friend class ArboreAkinator;
};
std::ostream& operator<<(std::ostream& os, const Nod& n) {
    if (n.esteFrunza() && n.getGhicitoare() != nullptr) {
        os << "[FRUNZA] " << *n.getGhicitoare();
    } else {
        os << "[INTREBARE] \"" << n.intrebare_ << "\"";
    }
    return os;
}

// CLASA 3: ArboreAkinator (Baza de Cunoaștere - R3 și Functii Complexe)
class ArboreAkinator {
private:
    std::unique_ptr<Nod> radacina_;
    static std::unique_ptr<Nod> cloneNodRecursiv(const Nod* nod_sursa); // Functie ajutatoare pentru Deep Copy
    std::size_t getAdancimeRecursiv(const Nod* nod_curent) const;
    std::size_t numaraNoduriRecursiv(const Nod* nod_curent) const;
    static std::optional<bool> raspunsLaBool(std::string raspuns_text);
public:
    ArboreAkinator() = default;
    explicit ArboreAkinator(std::unique_ptr<Nod> radacina_noua) : radacina_(std::move(radacina_noua)) {}
    // R3: Constructor de Copiere (Deep Copy)
    ArboreAkinator(const ArboreAkinator& other) : radacina_(cloneNodRecursiv(other.radacina_.get())) {}
    // R3: Operator=
    ArboreAkinator& operator=(const ArboreAkinator& other) {
        if (this != &other) {
            radacina_ = cloneNodRecursiv(other.radacina_.get());
        }
        return *this;
    }
    ~ArboreAkinator() = default;

    // Functii Membre Publice Nontriviale
    [[nodiscard]] std::size_t calculeazaAdancime() const; // 1/3
    [[nodiscard]] std::optional<std::string> determinaEntitatea(std::istream& flux_intrare) const; // 2/3 (Nume Modificat)
    [[nodiscard]] std::string numarNoduri() const; // 3/3

    [[nodiscard]] std::string inaltime() const; // Ajutatoare

    // Functii de I/O
    static std::unique_ptr<Nod> fromJSONnode(const json& date_nod);
    static ArboreAkinator dinJSON(const json& date_sursa);
    friend std::ostream& operator<<(std::ostream& os, const ArboreAkinator& arb);
};
std::unique_ptr<Nod> ArboreAkinator::cloneNodRecursiv(const Nod* nod_sursa) {
    if (!nod_sursa) return nullptr;
    Entitate* noua_entitate = nullptr;
    if (nod_sursa->ghicitoare_) {
        noua_entitate = new Entitate(*nod_sursa->ghicitoare_); // Copiere profundă Entitate
    }
    std::unique_ptr<Nod> da_copie = cloneNodRecursiv(nod_sursa->getDa());
    std::unique_ptr<Nod> nu_copie = cloneNodRecursiv(nod_sursa->getNu());
    return std::make_unique<Nod>(nod_sursa->intrebare_, std::move(da_copie), std::move(nu_copie), noua_entitate);
}
std::size_t ArboreAkinator::calculeazaAdancime() const { return getAdancimeRecursiv(radacina_.get()); }
std::size_t ArboreAkinator::getAdancimeRecursiv(const Nod* nod_curent) const {
    if (nod_curent == nullptr) return 0;
    return 1 + std::max(getAdancimeRecursiv(nod_curent->getDa()), getAdancimeRecursiv(nod_curent->getNu()));
}
std::string ArboreAkinator::inaltime() const { return std::to_string(calculeazaAdancime()); }
std::size_t ArboreAkinator::numaraNoduriRecursiv(const Nod* nod_curent) const {
    if (nod_curent == nullptr) return 0;
    return 1 + numaraNoduriRecursiv(nod_curent->getDa()) + numaraNoduriRecursiv(nod_curent->getNu());
}
std::string ArboreAkinator::numarNoduri() const { return std::to_string(numaraNoduriRecursiv(radacina_.get())); }
std::optional<bool> ArboreAkinator::raspunsLaBool(std::string raspuns_text) {
    auto lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
    std::transform(raspuns_text.begin(), raspuns_text.end(), raspuns_text.begin(), lower);
    if (raspuns_text == "da" || raspuns_text == "d" || raspuns_text == "y" || raspuns_text == "yes") return true;
    if (raspuns_text == "nu" || raspuns_text == "n" || raspuns_text == "no") return false;
    return std::nullopt;
}
[[nodiscard]] std::optional<std::string> ArboreAkinator::determinaEntitatea(std::istream& flux_intrare) const {
    if (!radacina_) return std::nullopt;
    Nod* nod_curent = radacina_.get();
    while (nod_curent && !nod_curent->esteFrunza()) {
        std::string raspuns_utilizator;
        if (!(flux_intrare >> raspuns_utilizator)) return std::nullopt;
        aDouaBibliotecaExterna(nod_curent->getIntrebare());
        auto decizie_yn = raspunsLaBool(raspuns_utilizator);
        if (!decizie_yn.has_value()) { return std::nullopt; }
        nod_curent = decizie_yn.value() ? nod_curent->getDa() : nod_curent->getNu();
    }
    if (!nod_curent || !nod_curent->esteFrunza() || !nod_curent->getGhicitoare()) { return std::nullopt; }
    std::string confirmare_finala;
    if (!(flux_intrare >> confirmare_finala)) return nod_curent->getGhicitoare()->getNume();
    auto rezultat_confirmat = raspunsLaBool(confirmare_finala);
    if (rezultat_confirmat && rezultat_confirmat.value()) { return nod_curent->getGhicitoare()->getNume(); } else { return std::nullopt; }
}
std::unique_ptr<Nod> ArboreAkinator::fromJSONnode(const json& date_nod) {
    if (date_nod.contains("entitate")) {
        const auto& date_entitate = date_nod.at("entitate");
        auto entitate_noua = std::make_unique<Entitate>(
            date_entitate.at("nume").get<std::string>(),
            date_entitate.at("domeniu").get<std::string>(),
            date_entitate.at("tip").get<std::string>()
        );
        return std::make_unique<Nod>(std::move(entitate_noua));
    } else {
        auto text_intrebare = date_nod.at("intrebare").get<std::string>();
        auto ramura_da = fromJSONnode(date_nod.at("da"));
        auto ramura_nu = fromJSONnode(date_nod.at("nu"));
        return std::make_unique<Nod>(std::move(text_intrebare), std::move(ramura_da), std::move(ramura_nu), nullptr);
    }
}
ArboreAkinator ArboreAkinator::dinJSON(const json& date_sursa) {
    if (!date_sursa.contains("radacina")) { throw std::runtime_error("Fisier JSON invalid: Lipseste cheia 'radacina'."); }
    auto radacina_arbore = fromJSONnode(date_sursa.at("radacina"));
    return ArboreAkinator(std::move(radacina_arbore));
}
std::ostream& operator<<(std::ostream& os, const ArboreAkinator& arb) {
    os << "ArboreAkinator{ Adancime=" << arb.inaltime() << ", Noduri=" << arb.numarNoduri() << "}";
    return os;
}

// CLASA 4: ManagerRaspunsuri (Nume Modificat - Compunere și R3)
class ManagerRaspunsuri {
private:
    std::map<std::string, ArboreAkinator> arbori_cunoastere_; // Compunere: Stocheaza Arborii dupa tema
public:
    ManagerRaspunsuri() = default;
    ManagerRaspunsuri(const ManagerRaspunsuri& other) : arbori_cunoastere_(other.arbori_cunoastere_) {}
    ManagerRaspunsuri(ManagerRaspunsuri&& other) noexcept = default;
    ManagerRaspunsuri& operator=(ManagerRaspunsuri other) noexcept; // R3: Operator= (folosind swap)
    ~ManagerRaspunsuri() = default;

    // Functii
    void incarcaDinFisier(const std::string& tema, const std::string& cale_fisier);
    [[nodiscard]] bool existaTema(const std::string& tema) const { return arbori_cunoastere_.count(tema); }
    [[nodiscard]] const ArboreAkinator& getConst(const std::string& tema) const;

    // Operator<<
    friend std::ostream& operator<<(std::ostream& os, const ManagerRaspunsuri& r);
    friend void swap(ManagerRaspunsuri& first, ManagerRaspunsuri& second) noexcept;
};
void swap(ManagerRaspunsuri& first, ManagerRaspunsuri& second) noexcept { using std::swap; swap(first.arbori_cunoastere_, second.arbori_cunoastere_); }
ManagerRaspunsuri& ManagerRaspunsuri::operator=(ManagerRaspunsuri other) noexcept { swap(*this, other); return *this; }
void ManagerRaspunsuri::incarcaDinFisier(const std::string& tema, const std::string& cale_fisier) {
    std::ifstream fisier_input(cale_fisier);
    if (!fisier_input) throw std::runtime_error("Eroare la deschiderea fisierului: " + cale_fisier);
    json date_json;
    try {
        fisier_input >> date_json;
        arbori_cunoastere_.emplace(tema, ArboreAkinator::dinJSON(date_json));
    }
    catch (const json::parse_error& e) {
        throw std::runtime_error("Eroare la parsarea JSON din " + cale_fisier + ": " + e.what());
    }
}
[[nodiscard]] const ArboreAkinator& ManagerRaspunsuri::getConst(const std::string& tema) const {
    auto iter_gasit = arbori_cunoastere_.find(tema);
    if (iter_gasit == arbori_cunoastere_.end()) throw std::runtime_error("Tema inexistenta: " + tema);
    return iter_gasit->second;
}
std::ostream& operator<<(std::ostream& os, const ManagerRaspunsuri& r) {
    os << "ManagerRaspunsuri{ teme=[";
    bool first = true;
    for (const auto& kv : r.arbori_cunoastere_) {
        if (!first) os << ", ";
        os << kv.first << "(" << kv.second.numarNoduri() << " noduri)";
        first = false;
    }
    os << "] }";
    return os;
}

// CLASA 5: JocAkinator (Clasa de Nivel Înalt)
class JocAkinator {
private:
    ManagerRaspunsuri baza_cunoastere_; // Compunere: Detine Repozitoriul
    std::string tema_curenta_;
public:
    // Constructor de inițializare
    explicit JocAkinator(ManagerRaspunsuri repo, std::string temaInitiala)
        : baza_cunoastere_(std::move(repo)), tema_curenta_(std::move(temaInitiala)) {
        if (!baza_cunoastere_.existaTema(tema_curenta_)) { throw std::runtime_error("Tema initiala invalida."); }
    }
    JocAkinator(const JocAkinator& other) = default;
    JocAkinator& operator=(const JocAkinator& other) = default;
    ~JocAkinator() = default;

    void schimbaTema(const std::string& tema) {
        if (!baza_cunoastere_.existaTema(tema)) throw std::runtime_error("Tema inexistenta: " + tema);
        tema_curenta_ = tema;
    }
    // Funcționalitate complexă: rulează jocul citind dintr-un flux
    [[nodiscard]] std::optional<std::string> ruleazaSilențios(std::istream& flux_intrare) const {
        std::string tema_solicitata;
        if (!(flux_intrare >> tema_solicitata)) return std::nullopt;
        if (baza_cunoastere_.existaTema(tema_solicitata)) {
             // Apel catre functia determinEntitatea
             return baza_cunoastere_.getConst(tema_solicitata).determinaEntitatea(flux_intrare);
        }
        return std::nullopt;
    }
    [[nodiscard]] const ArboreAkinator& getArboreCurent() const { return baza_cunoastere_.getConst(tema_curenta_); }

    // Operator<<
    friend std::ostream& operator<<(std::ostream& os, const JocAkinator& j);
};
std::ostream& operator<<(std::ostream& os, const JocAkinator& j) {
    os << "JocAkinator(temaCurenta=" << j.tema_curenta_ << ", " << j.baza_cunoastere_ << ")";
    return os;
}

// MAIN — Scenariu de utilizare (Gata pentru Commit v0.1)
int main() {
    // 1. Deschiderea fluxului de ieșire către fișierul raspuns.txt
    std::ofstream fisier_raspuns("raspuns.txt");
    if (!fisier_raspuns.is_open()) {
        std::cerr << "Eroare Fatala: Nu s-a putut deschide fisierul raspuns.txt pentru scriere.\n";
        return 1;
    }

    // Optimizare I/O
    std::ios_base::sync_with_stdio(false);

    try {
        // --- 1. ÎNCĂRCARE BAZĂ DE DATE ---
        ManagerRaspunsuri repo_initial;
        // Incarcarea datelor din fisiere JSON
        repo_initial.incarcaDinFisier("tari", "tari_arbore.json");
        repo_initial.incarcaDinFisier("animale", "animale_arbore.json");
        repo_initial.incarcaDinFisier("vedete", "vedeta_arbore.json");

        // --- 2. TESTARE R3 PE REPOZITORIU (Copiere și Asignare) ---
        // Se testeaza constructorul de copiere si operatorul= (folosind deep copy)
        ManagerRaspunsuri repo_copie = repo_initial;
        ManagerRaspunsuri repo_asignare;
        repo_asignare = repo_copie;

        // --- 3. INIȚIALIZARE JOC ---
        JocAkinator joc_principal(std::move(repo_initial), "tari");

        // --- 4. PREGĂTIREA INTRARE EXPLICITĂ DIN FIȘIER ---
        const std::string CALE_INTRARE = "tastatura.txt";
        std::ifstream fisier_intrare(CALE_INTRARE);

        if (!fisier_intrare.is_open()) {
            throw std::runtime_error("Eroare la deschiderea fisierului de intrare: " + CALE_INTRARE);
        }

        std::cout << "DEBUG: Rulare Akinator. Citire din '" << CALE_INTRARE << "'. Rezultatul final va fi in raspuns.txt\n";

        // Rularea jocului si apelarea functiei determinaEntitatea
        auto rezultat = joc_principal.ruleazaSilențios(fisier_intrare);

        fisier_intrare.close();

        // --- 5. AFIȘARE REZULTAT FINAL ÎN FIȘIER ---
        if (rezultat.has_value()) {
            fisier_raspuns << rezultat.value() << "\n";
        } else {
            fisier_raspuns << "NECUNOSCUT\n";
        }

        // Verificari finale folosind operator<<
        fisier_raspuns << "\n--- Verificari Structura ---\n";
        fisier_raspuns << "Repo copiat: " << repo_copie << "\n"; // Testeaza operator<< si integritatea copiei
        fisier_raspuns << "Arbore curent (Adancime): " << joc_principal.getArboreCurent().inaltime() << "\n";

        fisier_raspuns.close();

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Eroare Fatala: " << ex.what() << "\n";
        fisier_raspuns.close();
        return 1;
    }
}