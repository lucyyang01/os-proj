use std::env;
use std::fs::metadata;
use std::net::{Ipv4Addr, SocketAddrV4};
use std::path::Path;

use crate::args;

use crate::http::*;
use crate::stats::*;

use clap::Parser;
use tokio::net::{TcpListener, TcpStream};
use tokio::io::AsyncReadExt;

use anyhow::Result;
use tokio::fs::File;


pub fn main() -> Result<()> {
    // Configure logging
    // You can print logs (to stderr) using
    // `log::info!`, `log::warn!`, `log::error!`, etc.
    env_logger::Builder::new()
        .filter_level(log::LevelFilter::Info)
        .init();

    // Parse command line arguments
    let args = args::Args::parse();

    // Set the current working directory
    env::set_current_dir(&args.files)?;

    // Print some info for debugging
    log::info!("HTTP server initializing ---------");
    log::info!("Port:\t\t{}", args.port);
    log::info!("Num threads:\t{}", args.num_threads);
    log::info!("Directory:\t\t{}", &args.files);
    log::info!("----------------------------------");
    println!("hello sohom");

    // Initialize a thread pool that starts running `listen`
    tokio::runtime::Builder::new_multi_thread()
        .enable_all()
        .worker_threads(args.num_threads)
        .build()?
        .block_on(listen(args.port))
}

async fn listen(port: u16) -> Result<()> {
    // Hint: you should call `handle_socket` in this function.
    println!("listen");

    let addr = format!("0.0.0.0:{}", port).parse::<SocketAddrV4>()?;
    let listener = TcpListener::bind(&addr).await?;

    while let Ok((socket, _)) = listener.accept().await {
        tokio::spawn(handle_socket(socket));
    }
    Ok(())
}

// Handles a single connection via `socket`.
async fn handle_socket(mut socket: TcpStream) -> Result<()> {
    println!("Handle socket");

    //parse request
    let parsed = parse_request(&mut socket).await?; //parsed is request struct
    let file_path = format!(".{}", parsed.path);
    let f = File::open(&file_path).await?;
    
    let fp = parsed.path.clone();
    println!("here");
    let metadata = f.metadata().await?;
    println!("metadata: {:?}", metadata);  

    if metadata.is_dir() {
        println!("reached dir");
        //check if directory contains an index file
        //append a trailing slash
        let file_path = format!("{}/", fp);
        println!("filepath: {:?}", file_path);  
        let complete_idx_path = format_index(&fp);
        //format this into a paht
        let path = Path::new(&complete_idx_path);
        println!("filepath: {:?}", complete_idx_path);  
        println!("path exists: {:?}", path.exists());  
        if path.exists() {
            //respond with 200 ok and full contents of index file
            let path_str = path.as_os_str().to_str().unwrap();
            start_response(&mut socket, 200).await?;
            //open file
            let mut file = match File::open(path_str).await {
                Ok(file) => file,
                Err(_) => {
                    start_response(&mut socket, 404).await?;
                    return Ok(()); 
                }
            };
            let mime_type = get_mime_type(&fp);
            //println!("mime type: {:?}", mime_type);
            let metadata = file.metadata().await?;
            let filesize = metadata.len().to_string(); //this is type u64, convert to string
            //println!("file size: {:?}", filesize);
            start_response(&mut socket, 200).await?;
            let h1 = "Content-Type";
            send_header(&mut socket, &h1, &mime_type).await?;
            let h2 = "Content-Length";
            send_header(&mut socket, &h2, &filesize).await?;
            end_headers(&mut socket).await?;      
            loop {
                let mut buf = [0; 1024];
                //println!("We made it to before bytes_read!");
                let bytes_read = file.read(&mut buf).await?;
                //println!("bytes read: {:?}", bytes_read);
                //we finished reading
                //println!("{}",bytes_read < 1024);
                if bytes_read < 1024 {
                    socket.try_write(&buf[..bytes_read]);
                    break;
                }
                socket.try_write(&buf[..bytes_read]);
            }
        } else {
            //if index.html doesn't exist use format_href  
            //to generate formatted links to every file insdie the directory and then send it as an http response
            
            //list all files inside directory
            println!("INDEX DOESN'T EXIST IN DIR");  
            start_response(&mut socket, 200).await?;
            let mime_type = get_mime_type(&file_path);
            println!("MIME : {:?}", mime_type);  

            let h1 = "Content-Type";
            send_header(&mut socket, &h1, &mime_type).await?;
            end_headers(&mut socket).await?;     
            let mut entries = tokio::fs::read_dir(file_path).await?;
            //let mut buf = [0; 1024];
            while let Some(entry) = entries.next_entry().await? {
                //convert pathbuf to string
                let path_buf = entry.path();
                let path_buf_str = path_buf.into_os_string().into_string().unwrap();
                let borrow = format!("{}/", fp);
                let formatted = format_href(&borrow,&path_buf_str);
                // let h1 = "Content-Type";
                socket.try_write(&formatted.as_bytes());
                // send_header(&mut socket, &h1, &mime_type).await?;
            }
        }
    } else if metadata.is_file() {
        println!("reached file");

        let file_path = format!(".{}", fp);
        
        let mut file = match File::open(file_path).await {
            Ok(file) => file,
            Err(_) => {
                start_response(&mut socket, 404).await?;
                return Ok(()); 
            }
        };
    
        let mime_type = get_mime_type(&fp);
        //println!("mime type: {:?}", mime_type);
        let metadata = file.metadata().await?;
        let filesize = metadata.len().to_string(); //this is type u64, convert to string
        //println!("file size: {:?}", filesize);
        start_response(&mut socket, 200).await?;
        let h1 = "Content-Type";
        send_header(&mut socket, &h1, &mime_type).await?;
        let h2 = "Content-Length";
        send_header(&mut socket, &h2, &filesize).await?;
        end_headers(&mut socket).await?;      
        loop {
            let mut buf = [0; 1024];
            //println!("We made it to before bytes_read!");
            let bytes_read = file.read(&mut buf).await?;
            //println!("bytes read: {:?}", bytes_read);
            //we finished reading
            //println!("{}",bytes_read < 1024);
            if bytes_read < 1024 {
                socket.try_write(&buf[..bytes_read]);
                break;
            }
            socket.try_write(&buf[..bytes_read]);
    
        }
    } else {
        return Ok(())
    }
    //println!("finished");
    Ok(())
}

// // Handles a single connection via `socket`.
// async fn handle_socket(mut socket: TcpStream) -> Result<()> {
//     //println!("Handle socket");
//     //parse request
//     let parsed = parse_request(&mut socket).await?; //parsed is request struct
//     let fp = parsed.path.clone();
//     println!("fp: {:?}", fp);  
//     let file_path = format!(".{}", fp);
//     //println!("file_path: {:?}", file_path);  
//     //let mut file = File::open(file_path).await?;
    
//     let mut file = match File::open(file_path).await {
//         Ok(file) => file,
//         Err(_) => {
//             start_response(&mut socket, 404).await?;
//             return Ok(()); 
//         }
//     };

//     let mime_type = get_mime_type(&fp);
//     //println!("mime type: {:?}", mime_type);
//     let metadata = file.metadata().await?;
//     let filesize = metadata.len().to_string(); //this is type u64, convert to string
//     //println!("file size: {:?}", filesize);
//     start_response(&mut socket, 200).await?;
//     let h1 = "Content-Type";
//     send_header(&mut socket, &h1, &mime_type).await?;
//     let h2 = "Content-Length";
//     send_header(&mut socket, &h2, &filesize).await?;
//     end_headers(&mut socket).await?;      
//     loop {
//          //fs::read returns a vector already
//         let mut buf = [0; 1024];
//         //println!("We made it to before bytes_read!");
//         let bytes_read = file.read(&mut buf).await?;
//         //println!("bytes read: {:?}", bytes_read);
//         //we finished reading
//         //println!("{}",bytes_read < 1024);
//         if bytes_read < 1024 {
//             socket.try_write(&buf[..bytes_read]);
//             break;
//         }
//         socket.try_write(&buf[..bytes_read]);

//     }
//     //println!("finished");
//     Ok(())
// }

//if path.exists()

// You are free (and encouraged) to add other funtions to this file.
// You can also create your own modules as you see fit.