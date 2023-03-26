use std::{net, io::Read, path::Path};

fn main() -> Result<(), std::io::Error> {
    let listener = net::TcpListener::bind("0.0.0.0:8001")?;
    std::fs::create_dir_all(Path::new("./recordings/"))?;

    for stream in listener.incoming() {
        match stream {
            Ok(mut stream) => {
                // this server is intended to handle exactly one client, hence the fact this runs
                // in the main thread
                
                println!("got connection");
                let mut size_raw = [0u8; 4];

                // read an integer
                stream.read_exact(&mut size_raw)?;
                let size = i32::from_be_bytes(size_raw);

                println!("Got size {}", size);
                if size < 0 {
                    println!("Negative size!");
                    continue;
                }
                // read a jpeg of that size
                let mut jpeg = Vec::<u8>::with_capacity(size as usize);

                // read an integer
                stream.read_exact(jpeg.as_mut_slice())?;

                let _ = std::fs::write("test.jpeg", jpeg);
                println!("wrote jpeg");

            },
            Err(e) => {
                eprintln!("got connection error {}",e);
            }
        }
    }

    Ok(())
}
