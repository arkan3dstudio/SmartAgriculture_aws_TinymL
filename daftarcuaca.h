
struct CuacaData {
  int idCuaca;  // ID dari daftarCuaca berdasarkan kode cuaca
  String waktu;
  String nama_kota;
  String negara;
  String cuaca_group;
  String cuaca_deskripsi;
  int code;
  float suhu;
  float feels_like;
  float temp_min;
  float temp_max;
  int pressure;
  int humidity;
  int visibility;
  float wind_speed;
  int wind_deg;
  float gust;
  float rain1h;
  int cloud;
  int sea_level;
  int grnd_level;
  String waktu_sunrise;
  String waktu_sunset;
  String arah_angin;
  String keterangan_lokasi;
  float suhuBMP;
  float lembabBMP;
  float tekanBMP;
  float tinggiBMP;
  float anginBMP;
  float ketinggian_air_deteksi; 
  bool valid;
};


struct Cuaca{
  int id;
  int code;
  String group;
  String deskripsi;
};

Cuaca daftarCuaca[] = {
  {1,200,"Badai Petir","Badai Petir & Hujan ringan"},
  {2,201,"Badai Petir","Badai Petir & Hujan "},
  {3,202,"Badai Petir","Badai Petir & Hujan Lebat"},
  {4,210,"Badai Petir","Badai Petir Ringan"},
  {5,211,"Badai Petir","Badai Petir"},
  {6,212,"Badai Petir","Badai Petir Yang Berat"},
  {7,221,"Badai Petir","Badai Petir & Gemuruh"},
  {8,230,"Badai Petir","Badai Petir & Gerimis Ringan"},
  {9,231,"Badai Petir","Badai Petir & Gerimis"},
  {10,232,"Badai Petir","Badai Petir & Gerimis Lebat "},
  {11,300,"Gerimis","Gerimis Ringan"},
  {12,301,"Gerimis","Gerimis"},
  {13,302,"Gerimis","Gerimis Lebat"},
  {14,310,"Gerimis","Hujan Gerimis Ringan"},
  {15,311,"Gerimis","Hujan Gerimis"},
  {16,312,"Gerimis","Hujan Gerimis Lebat"},
  {17,313,"Gerimis","Hujan Turun & Gerimis"},
  {18,314,"Gerimis","Hujan Deras & Gerimis"},
  {19,321,"Gerimis","Hujan Gerimis"},
  {20,500,"Hujan","Hujan Ringan"},
  {21,501,"Hujan","Hujan Sedang"},
  {22,502,"Hujan","Hujan Intensitas Lebat"},
  {23,503,"Hujan","Hujan Sangat Lebat"},
  {24,504,"Hujan","Hujan Ekstream"},
  {25,511,"Hujan","Hujan Beku"},
  {26,520,"Hujan","Hujan Intensitas Ringan"},
  {27,521,"Hujan","Hujan Turun"},
  {28,522,"Hujan","Hujan Intensitas Lebat"},
  {29,531,"Hujan","Hujan Turun Tidak Merata"},
  {30,701,"Kabut/Mist","Berkabut Tipis"},
  {31,711,"Asap","Berasap"},
  {32,721,"Kabut/Haze","Berkabut Tipis"},
  {33,731,"Debu","Berdebu & Angin"},
  {34,741,"Kabut/Fog","Berkabut Tebal"},
  {35,751,"Pasir","Berpasir"},
  {36,761,"Debu","Berdebu"},
  {37,762,"Debu","Berdebu Vulcanik"},
  {38,771,"Badai","Badai"},
  {39,781,"Tornado","Badai Tornado"},
  {40,800,"Cerah","Langit Cerah"},
  {41,801,"Berawan","Sedikit Awan 11-25%"},
  {42,802,"Berawan","Awan Tersebar 25-50%"},
  {43,803,"Berawan","Awan Pecah 51-84%"},
  {44,804,"Berawan","Awan Mendung 85-100%"}
};
