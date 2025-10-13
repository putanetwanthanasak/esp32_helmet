// ฟังก์ชันจัดการเมื่อกดปุ่ม Register
document.addEventListener("DOMContentLoaded", () => {
  const form = document.getElementById("myRegister");
  if (form) {
    form.addEventListener("submit", function(event) {
      event.preventDefault();

      const data = {
        firstname: form.firstname.value,
        lastname: form.lastname.value,
        gender: form.gender.value,
        bday: form.bday.value,
        bloodgroup: form.bloodgroup.value,
        disease: form.disease.value,
        allergy: form.allergy.value,
        medication: form.medication.value,
        emergencyname: form.emergencyname.value,
        emergencyphone: form.emergencyphone.value
      };

      localStorage.setItem("profileData", JSON.stringify(data));
      window.location.href = "profile.html";
    });
  }

  // ถ้าอยู่ในหน้า profile.html ให้โหลดข้อมูลมาแสดง
  const profileDiv = document.getElementById("profileData");
  if (profileDiv) {
    const data = JSON.parse(localStorage.getItem("profileData"));

    if (data) {
      profileDiv.innerHTML = `
        <div class="info"><span>Name:</span> ${data.firstname} ${data.lastname}</div>
        <div class="info"><span>Gender:</span> ${data.gender}</div>
        <div class="info"><span>Birthday:</span> ${data.bday}</div>
        <div class="info"><span>Blood Group:</span> ${data.bloodgroup}</div>
        <div class="info"><span>Underlying Diseases:</span> ${data.disease || '-'}</div>
        <div class="info"><span>Allergies:</span> ${data.allergy || '-'}</div>
        <div class="info"><span>Current Medications:</span> ${data.medication || '-'}</div>
        <div class="info"><span>Emergency Contact:</span> ${data.emergencyname}</div>
        <div class="info"><span>Emergency Phone:</span> ${data.emergencyphone}</div>
      `;
    } else {
      profileDiv.innerHTML = `<p>No profile data found. Please <a href='register.html'>register</a> first.</p>`;
    }
  }
});
