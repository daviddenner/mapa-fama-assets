// Testes da parte da página que roda no PC: MD5, login Digest, JSON, tabela
// em texto e conferência da memória.
//   g++ -std=c++17 -Wall -I../sino_gps teste_web.cpp -o teste_web && ./teste_web
#include <cstdio>
#include <cstring>
#include <string>

#include "agenda.h"
#include "armazenamento.h"
#include "web_util.h"

static int falhas = 0;
#define CHECA(cond)                                          \
  do {                                                       \
    if (!(cond)) {                                           \
      std::printf("FALHOU linha %d: %s\n", __LINE__, #cond); \
      falhas++;                                              \
    }                                                        \
  } while (0)

static std::string md5(const std::string &s) {
  Md5 m;
  md5Iniciar(m);
  md5Somar(m, s.data(), s.size());
  char h[33];
  md5Hex(m, h);
  return h;
}

// Monta o cabeçalho Authorization como um navegador faria.
static std::string autorizacao(const char *usuario, const char *senha, const char *nonce, const char *metodo,
                               const char *uri) {
  char ha1[33], ha2[33], resp[33];
  md5DePartes(ha1, usuario, DIGEST_REINO, senha);
  md5DePartes(ha2, metodo, uri);
  md5DePartes(resp, ha1, nonce, "00000001", "0a4f113b", "auth", ha2);
  return std::string("Digest username=\"") + usuario + "\", realm=\"" DIGEST_REINO "\", nonce=\"" + nonce +
         "\", uri=\"" + uri + "\", algorithm=MD5, response=\"" + resp +
         "\", opaque=\"sino\", qop=auth, nc=00000001, cnonce=\"0a4f113b\"";
}

int main() {
  // ------------------------------------------------------------------ MD5
  CHECA(md5("") == "d41d8cd98f00b204e9800998ecf8427e");
  CHECA(md5("abc") == "900150983cd24fb0d6963f7d28e17f72");
  CHECA(md5("The quick brown fox jumps over the lazy dog") == "9e107d9d372bb6826bd81d3542a419d6");
  // tamanhos nas bordas do bloco de 64 bytes (o enchimento muda)
  CHECA(md5(std::string(55, 'a')) == "ef1772b6dff9a122358552954ad0df65");
  CHECA(md5(std::string(56, 'a')) == "3b0c8ac703f828b04c6c197006d17218");
  CHECA(md5(std::string(57, 'a')) == "652b906d60af96844ebd21b674f35e93");
  CHECA(md5(std::string(63, 'a')) == "b06521f39153d618550606be297466d5");
  CHECA(md5(std::string(64, 'a')) == "014842d480b571495a4a0363793f7367");
  CHECA(md5(std::string(65, 'a')) == "c743a45e0d2e6a95cb859adae0248435");
  CHECA(md5(std::string(1000, 'a')) == "cabe45dcc9ae5b66ba86600cca6b8ba8");

  // Exemplo da RFC 2617
  char ha1[33], ha2[33], r[33];
  md5DePartes(ha1, "Mufasa", "testrealm@host.com", "Circle Of Life");
  md5DePartes(ha2, "GET", "/dir/index.html");
  md5DePartes(r, ha1, "dcd98b7102dd2f0e8b11d0f600bfb0c093", "00000001", "0a4f113b", "auth", ha2);
  CHECA(!strcmp(ha1, "939e7578ed9e3c518a452acee763bce9") && !strcmp(ha2, "39aff3a2bab6126f332b942af96d3366"));
  CHECA(!strcmp(r, "6629fae49393a05397450978507c4ef1"));

  // --------------------------------------------------------- login Digest
  const char *seg = "0123456789abcdef";
  char nonce[25];
  gerarNonce(seg, 1000, nonce);
  CHECA(strlen(nonce) == 24 && !strncmp(nonce, "000003e8", 8));
  CHECA(nonceValido(nonce, seg, 1000) && nonceValido(nonce, seg, 1000 + 3600));
  CHECA(!nonceValido(nonce, seg, 1000 + DIGEST_VALIDADE_S + 1));  // venceu
  CHECA(!nonceValido(nonce, "outro-segredo", 1000));               // sino reiniciou
  char adulterado[25];
  strcpy(adulterado, nonce);
  adulterado[0] = '1';  // muda a hora de emissão
  CHECA(!nonceValido(adulterado, seg, 1000));

  char desafio[200];
  desafioDigest(seg, 1000, false, desafio, sizeof desafio);
  CHECA(strstr(desafio, "Digest realm=\"Sino IFMS\", qop=\"auth\"") && strstr(desafio, nonce) && !strstr(desafio, "stale"));
  desafioDigest(seg, 1000, true, desafio, sizeof desafio);
  CHECA(strstr(desafio, "stale=TRUE") != nullptr);

  std::string a = autorizacao("admin", "segredo1", nonce, "GET", "/api/estado");
  CHECA(verificarDigest(a.c_str(), "GET", "/api/estado", "admin", "segredo1", seg, 1005) == DIGEST_OK);
  CHECA(verificarDigest(a.c_str(), "GET", "/api/estado", "admin", "outra", seg, 1005) == DIGEST_NEGADO);
  CHECA(verificarDigest(a.c_str(), "POST", "/api/estado", "admin", "segredo1", seg, 1005) == DIGEST_NEGADO);
  CHECA(verificarDigest(a.c_str(), "GET", "/api/horarios", "admin", "segredo1", seg, 1005) == DIGEST_NEGADO);
  CHECA(verificarDigest(a.c_str(), "GET", "/api/estado", "root", "segredo1", seg, 1005) == DIGEST_NEGADO);
  CHECA(verificarDigest(a.c_str(), "GET", "/api/estado", "admin", "segredo1", seg, 1000 + DIGEST_VALIDADE_S + 9) ==
        DIGEST_VENCIDO);
  CHECA(verificarDigest(a.c_str(), "GET", "/api/estado", "admin", "segredo1", "reiniciou", 1005) == DIGEST_VENCIDO);
  a = autorizacao("admin", "segredo1", nonce, "GET", "/api/estado?x=1");  // caminho com "?..."
  CHECA(verificarDigest(a.c_str(), "GET", "/api/estado", "admin", "segredo1", seg, 1005) == DIGEST_OK);
  CHECA(verificarDigest("Basic YWRtaW46c2VncmVkbzE=", "GET", "/", "admin", "segredo1", seg, 1005) == DIGEST_NEGADO);
  CHECA(verificarDigest("Digest lixo", "GET", "/", "admin", "segredo1", seg, 1005) == DIGEST_NEGADO);
  {  // Digest antigo (RFC 2069, sem qop)
    char h1[33], h2[33], resp[33];
    md5DePartes(h1, "admin", DIGEST_REINO, "segredo1");
    md5DePartes(h2, "GET", "/");
    md5DePartes(resp, h1, nonce, h2);
    std::string c = std::string("Digest username=\"admin\", realm=\"" DIGEST_REINO "\", nonce=\"") + nonce +
                    "\", uri=\"/\", response=\"" + resp + "\"";
    CHECA(verificarDigest(c.c_str(), "GET", "/", "admin", "segredo1", seg, 1005) == DIGEST_OK);
  }
  char v[40];
  CHECA(parametroDigest("cnonce=\"x\", nonce=\"y\"", "nonce", v, sizeof v) && !strcmp(v, "y"));
  CHECA(parametroDigest("qop=auth, nc=00000001", "nc", v, sizeof v) && !strcmp(v, "00000001"));
  CHECA(!parametroDigest("username=\"aberto", "username", v, sizeof v));

  // ----------------------------------------------------------------- JSON
  char j[64];
  jsonTexto(j, sizeof j, "Rede \"IFMS\"\\2\n");
  CHECA(!strcmp(j, "Rede \\\"IFMS\\\"\\\\2\\u000a"));

  // ------------------------------------------------------ tabela em texto
  Toque t[MAX_HORARIOS];
  char erro[80];
  int n = lerTabela(" 09:30,62,1500,1;\n07:00,62,2500,2 ;07:00,64,1000,1", t, MAX_HORARIOS, 30000, erro, sizeof erro);
  CHECA(n == 3 && t[0].hora == 7 && t[0].dias == 62 && t[1].hora == 7 && t[1].dias == 64 && t[2].hora == 9);
  CHECA(t[0].duracaoMs == 2500 && t[0].vezes == 2);
  CHECA(lerTabela("", t, MAX_HORARIOS, 30000, erro, sizeof erro) == 0);  // tabela vazia vale
  struct {
    const char *txt, *erro;
  } ruins[] = {
      {"07:00,62,1500", "formato"},          {"07:00,62,1500,1x", "formato"},
      {"24:00,62,1500,1", "hora invalida"},  {"07:60,62,1500,1", "hora invalida"},
      {"07:00,0,1500,1", "pelo menos um dia"}, {"07:00,128,1500,1", "pelo menos um dia"},
      {"07:00,62,50,1", "duracao"},          {"07:00,62,30001,1", "duracao"},
      {"07:00,62,1500,0", "batidas"},        {"07:00,62,1500,11", "batidas"},
      {"07:00,2,1500,1;07:00,6,1500,1", "duas vezes"},
  };
  for (auto &rr : ruins) {
    const int k = lerTabela(rr.txt, t, MAX_HORARIOS, 30000, erro, sizeof erro);
    CHECA(k == -1 && strstr(erro, rr.erro));
    if (k != -1 || !strstr(erro, rr.erro)) std::printf("   (texto \"%s\" -> \"%s\")\n", rr.txt, erro);
  }
  std::string muitos;
  for (int i = 0; i <= MAX_HORARIOS; i++) {
    char b[24];
    snprintf(b, sizeof b, "%02d:%02d,62,1500,1;", 6 + i / 60, i % 60);
    muitos += b;
  }
  CHECA(lerTabela(muitos.c_str(), t, MAX_HORARIOS, 30000, erro, sizeof erro) == -1 && strstr(erro, "no maximo"));

  char txt[MAX_HORARIOS * 24 + 8];
  CHECA(escreverTabela(HORARIOS, NUM_HORARIOS, txt, sizeof txt) > 0);
  CHECA(!strncmp(txt, "07:00,62,1500,1;07:50,62,1500,1;", 32));
  n = lerTabela(txt, t, MAX_HORARIOS, 30000, erro, sizeof erro);
  bool iguais = n == NUM_HORARIOS;
  for (int i = 0; iguais && i < n; i++)
    iguais = t[i].hora == HORARIOS[i].hora && t[i].minuto == HORARIOS[i].minuto && t[i].dias == HORARIOS[i].dias &&
             t[i].duracaoMs == HORARIOS[i].duracaoMs && t[i].vezes == HORARIOS[i].vezes;
  CHECA(iguais);  // ida e volta igual
  CHECA(memcmp(t, HORARIOS, sizeof(HORARIOS)) == 0);  // até os bytes de preenchimento
  char pequeno[10];
  CHECA(escreverTabela(HORARIOS, NUM_HORARIOS, pequeno, sizeof pequeno) == 0);

  // --------------------------------------------------- memória (EEPROM)
  CHECA(crc32((const uint8_t *)"123456789", 9) == 0xCBF43926UL);
  Configuracao c;
  memset(&c, 0, sizeof c);
  c.nHorarios = 2;
  memcpy(c.horarios, HORARIOS, 2 * sizeof(Toque));
  copiarTexto(c.apSenha, sizeof c.apSenha, "senha-do-wifi");
  copiarTexto(c.adminSenha, sizeof c.adminSenha, "senha-pagina");
  selarConfig(c);
  CHECA(configValida(c));
  Configuracao d = c;
  d.horarios[1].minuto ^= 1;  // um bit trocado na memória
  CHECA(!configValida(d));
  d = c;
  memset(d.adminSenha, 'x', sizeof d.adminSenha);  // texto sem fim
  selarConfig(d);
  CHECA(!configValida(d));
  Configuracao vazia;
  memset(&vazia, 0xFF, sizeof vazia);  // flash apagada
  CHECA(!configValida(vazia));

  if (falhas == 0) std::printf("OK: todos os testes passaram\n");
  return falhas ? 1 : 0;
}
