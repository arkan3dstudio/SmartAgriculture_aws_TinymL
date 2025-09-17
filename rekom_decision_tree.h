
String kategorikanSuhu(float suhu) {
  if (suhu < 20) return "<20°C";
  else if (suhu <= 30) return "20–30°C";
  else return ">30°C";
}

String kategorikanLembap(int kelembapan) {
  if (kelembapan < 50) return "<50%";
  else if (kelembapan <= 85) return "50–85%";
  else return ">85%";
}

String kategorikanAngin(float kecepatan) {
  if (kecepatan <= 1.5) return "0.0–1.5 m/s (Tenang)";
  else if (kecepatan <= 5.4) return "1.6–5.4 m/s (Normal)";
  else if (kecepatan <= 10.7) return "5.5–10.7 m/s (Kencang)";
  else return ">10.7 m/s (Ekstrem)";
}

String grupCuacaFromId(int idCuaca) {
  if (idCuaca >= 1 && idCuaca <= 10) return "Badai Petir";
  if (idCuaca >= 11 && idCuaca <= 19) return "Gerimis";
  if (idCuaca >= 20 && idCuaca <= 29) return "Hujan";
  if (idCuaca >= 30 && idCuaca <= 34 || idCuaca == 32) return "Kabut";
  if (idCuaca == 31) return "Asap";
  if (idCuaca == 35 || idCuaca == 36 || idCuaca == 37) return "Debu";
  if (idCuaca == 38 || idCuaca == 39) return "Badai";
  if (idCuaca == 40) return "Cerah";
  if (idCuaca >= 41 && idCuaca <= 44) return "Berawan";
  return "Tidak Dikenali";
}

String klasifikasiRekomendasi(int idCuaca, float suhu, int kelembapan, float angin) {
  String cuaca = grupCuacaFromId(idCuaca);
  String suhuKategori = kategorikanSuhu(suhu);
  String lembapKategori = kategorikanLembap(kelembapan);
  String anginKategori = kategorikanAngin(angin);

  // CUACA CERAH
  if (cuaca == "Cerah") {
    if (suhu > 35 && kelembapan < 60) {
      return "Perbanyak irigasi. Gunakan mulsa jika memungkinkan. Hindari pemupukan siang hari. Pantau gejala kekeringan daun.";
    } else if (suhu > 32) {
      return "Lakukan irigasi rutin pagi/sore. Waspadai stres panas, terutama saat tanaman berbunga.";
    } else {
      return "Waktu ideal untuk pemupukan, pengendalian gulma, dan panen.";
    }
  }
  // CUACA HUJAN
  else if (cuaca == "Hujan") {
    if (suhu < 18) return "Lindungi benih dari genangan dan suhu rendah. Pakai varietas toleran. Periksa drainase.";
    else return "Tunda pemupukan dan penyemprotan pestisida cair. Periksa drainase sawah agar tidak tergenang.";
  }
  // CUACA BERAWAN
  else if (cuaca == "Berawan") {
    if (lembapKategori == ">85% (Risiko penyakit/jamur tinggi)") {
      return "Waspadai penyakit blas dan hawar daun. Kurangi kepadatan tanaman. Tingkatkan sirkulasi udara.";
    } else {
      return "Aktivitas normal. Tetap pantau perubahan cuaca mendadak.";
    }
  }
  // KABUT / ASAP
  else if (cuaca == "Kabut" || cuaca == "Asap") {
    return "Pantau gejala penyakit jamur, seperti blast dan hawar. Kurangi kegiatan lapangan jika visibilitas rendah.";
  }
  // GERIMIS
  else if (cuaca == "Gerimis") {
    if (lembapKategori == ">85% (Risiko penyakit/jamur tinggi)") {
      return "Waspadai serangan penyakit jamur. Hindari penyemprotan pestisida cair. Pantau daun dan batang.";
    } else {
      return "Lanjutkan aktivitas ringan. Perhatikan perkembangan gulma dan hama.";
    }
  }
  // BADAI PETIR / BADAI / TORNADO
  else if (cuaca == "Badai Petir" || cuaca == "Badai" || idCuaca == 39) {
    if (angin > 10.0) return "Tunda seluruh aktivitas. Amankan peralatan dan benih. Risiko rebah dan kerusakan tanaman sangat tinggi.";
    else return "Tunda aktivitas lapangan. Pastikan saluran air lancar. Segera evaluasi kondisi setelah cuaca membaik.";
  }
  // DEBU / PASIR
  else if (cuaca == "Debu" || cuaca == "Pasir") {
    if (angin > 10.0) return "Lindungi tanaman dari pengikisan angin. Gunakan pelindung angin atau barier alami.";
    else return "Siram secara berkala. Gunakan mulsa untuk menjaga kelembapan tanah.";
  }

  // DEFAULT
  return "Lanjutkan praktik budidaya padi sesuai rekomendasi GAP (Good Agricultural Practice).";
}
